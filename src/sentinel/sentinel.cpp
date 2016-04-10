#include "Windows.h"
#include <codecvt>
#include <iostream>

#include "common.h"
#include "utf8everywhere.h"

#include <Dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

void make_minidump(cdmp::crash_data *d) {
  // prepare file name
  char name[MAX_PATH];
  {
    SYSTEMTIME t;
    GetSystemTime(&t);
    wsprintfA(name, "crash_dump_%4d%02d%02d_%02d%02d%02d.dmp", t.wYear,
              t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
  }

  auto dump_file = CreateFileA(name, GENERIC_WRITE, FILE_SHARE_READ, 0,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  if (dump_file == INVALID_HANDLE_VALUE)
    return;

  EXCEPTION_POINTERS excp = {};

  auto proc_handle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, d->proc_id);
  auto thread_handle = OpenThread(THREAD_ALL_ACCESS, TRUE, d->thread_id);

  auto mdt =
      (MINIDUMP_TYPE)(MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo |
                      MiniDumpWithHandleData | MiniDumpWithThreadInfo |
                      MiniDumpWithUnloadedModules);

  MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
  exceptionInfo.ThreadId = d->thread_id;
  exceptionInfo.ExceptionPointers = d->excp_ptrs;
  exceptionInfo.ClientPointers = TRUE;

  auto dumped = MiniDumpWriteDump(proc_handle, d->proc_id, dump_file, mdt,
                                  &exceptionInfo, nullptr, nullptr);

  CloseHandle(dump_file);

  return;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "USAGE: sentinel.exe [name of the event] [name of the MMF]"
              << std::endl;
    return 1;
  }
  std::string event_name(argv[1]);
  std::string mmf_name(argv[2]);

  // open shared memory
  auto map_file =
      OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, widen(mmf_name).c_str());
  if (!map_file) {
    std::cerr << "Sentinel: Could not open MMF (" << mmf_name << ")."
              << std::endl;
    std::cerr << "GetLastError(): " << GetLastError() << std::endl;
    return 1;
  }
  char *mmf = reinterpret_cast<char *>(
      MapViewOfFile(map_file, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE));

  std::cout << "Sentinel started, waiting for crash event." << std::endl;

  // open crash event
  auto crash_event =
      OpenEventW(EVENT_ALL_ACCESS, FALSE, widen(event_name).c_str());
  if (!crash_event) {
    std::cerr << "Sentinel: Could not open named event (" << mmf_name << ")."
              << std::endl;
    std::cerr << "GetLastError(): " << GetLastError() << std::endl;
    return 1;
  }

  // ... and wait
  WaitForSingleObject(crash_event, INFINITE);

  // if we are here, we got the crash signal from the other process
  // (which also means that the MMF contains the crash info we
  // need)
  std::cout << "Crash detected. Creating dump..." << std::endl;

  auto data = reinterpret_cast<cdmp::crash_data *>(mmf);
  make_minidump(data);

  // signal that writing the crash dump was finished.
  SetEvent(crash_event);

  std::cout
      << "Dump written, signalling to crashed process. Press any key to close."
      << std::endl;
  std::cin.ignore();

  return 0;
}