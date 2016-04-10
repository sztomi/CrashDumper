#include "common.h"
#include "utf8everywhere.h"
#include <iostream>
#include <string>
#include <sstream>

const char *crash_id = "ca_crash";
const char *mmf_name = "ca_crash_mmf";
HANDLE g_crash_event;
char *g_mmf;

LONG WINAPI unhandled_filter(EXCEPTION_POINTERS *excp_ptrs) { 
  // collect the data for the sentinel
  cdmp::crash_data d;
  d.excp_ptrs = excp_ptrs;
  d.thread_id = GetCurrentThreadId();
  d.proc_id = GetCurrentProcessId();
  memcpy(g_mmf, &d, sizeof(d));

  // signal to the sentinel that the mmf has data
  SetEvent(g_crash_event);

  // wait until the sentinel finishes
  WaitForSingleObject(g_crash_event, INFINITE);

  // this results in a crash window by the OS
  return EXCEPTION_CONTINUE_SEARCH; 
}

bool init_sentinel() {
  // create synchronization event
  g_crash_event = CreateEventW(NULL, FALSE, FALSE, widen(crash_id).c_str());
  if (!g_crash_event) {
    std::cerr << "Could not initialize crash event object." << std::endl;
  }

  // create shared memory (through MMF)
  auto map_file =
      CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                         BUF_SIZE, widen(mmf_name).c_str());
  if (!map_file) {
    std::cerr << "Could not open MMF (" << crash_id << ")" << std::endl;
    std::cerr << "GetLastError(): " << GetLastError() << std::endl;
    return false;
  }

  g_mmf = reinterpret_cast<char *>(
      MapViewOfFile(map_file, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE));

  // start sentinel.exe
  STARTUPINFOW info = {sizeof(info)};
  PROCESS_INFORMATION processInfo;
  std::stringstream ss;
  ss << "sentinel.exe " << crash_id << " " << mmf_name;
  if (CreateProcessW(widen("sentinel.exe").c_str(),
                     const_cast<wchar_t *>(widen(ss.str()).c_str()), NULL, NULL,
                     TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &info,
                     &processInfo) == 0) {
    return false;
  }

  SetUnhandledExceptionFilter(unhandled_filter);
  return true;
}

void artifical_crash() {
  std::cout << "Crashing in " << std::endl;
  for (int i = 5; i > 0; --i) {
    std::cout << i << std::endl;
    Sleep(1000);
  }
  std::cout << "CRASHING NOW" << std::endl;

  int *x = nullptr;
  *x = 1;
}

int main() {
  if (!init_sentinel()) {
    std::cerr << "Could not start sentinel process." << std::endl;
    return 1;
  }

  artifical_crash();

  return 0; // well this won't happen due to the crash
}