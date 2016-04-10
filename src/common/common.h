#pragma once

#ifndef CRASHDUMPER_COMMON_H_
#define CRASHDUMPER_COMMON_H_

#include "Windows.h"

const size_t BUF_SIZE = 256;

namespace cdmp {
  struct crash_data {
    EXCEPTION_POINTERS *excp_ptrs;
    DWORD thread_id;
    DWORD proc_id;
  };
}

#endif