// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "time.h"
#include "libkernel.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <thread>

#ifdef _WIN64
#include <windows.h>
#endif

namespace Core {
namespace Libraries {
namespace Kernel {

// Boot time reference
static auto g_boot_time = std::chrono::high_resolution_clock::now();

u64 PS4_SYSV_ABI sceKernelGetTscFrequency() {
#ifdef _WIN64
  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  return freq.QuadPart;
#else
  return 1'000'000'000ULL; // Fallback: nanosecond resolution
#endif
}

u64 PS4_SYSV_ABI sceKernelGetProcessTime() {
  auto now = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(now -
                                                               g_boot_time)
      .count();
}

u64 PS4_SYSV_ABI sceKernelGetProcessTimeCounter() {
#ifdef _WIN64
  LARGE_INTEGER pc;
  QueryPerformanceCounter(&pc);
  return pc.QuadPart;
#else
  return std::chrono::high_resolution_clock::now().time_since_epoch().count();
#endif
}

u64 PS4_SYSV_ABI sceKernelGetProcessTimeCounterFrequency() {
  return sceKernelGetTscFrequency();
}

u64 PS4_SYSV_ABI sceKernelReadTsc() { return sceKernelGetProcessTimeCounter(); }

s32 PS4_SYSV_ABI posix_clock_gettime(u32 clock_id, OrbisKernelTimespec *ts) {
  if (!ts)
    return -1;

#ifdef _WIN64
  switch (clock_id) {
  case ORBIS_CLOCK_REALTIME:
  case ORBIS_CLOCK_REALTIME_PRECISE:
  case ORBIS_CLOCK_REALTIME_FAST:
  case ORBIS_CLOCK_SECOND: {
    FILETIME ft;
    GetSystemTimePreciseAsFileTime(&ft);
    constexpr u64 DELTA_EPOCH = 116444736000000000ULL;
    u64 ticks = ((u64)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    u64 ns100 = ticks - DELTA_EPOCH;
    ts->tv_sec = (s64)(ns100 / 10'000'000ULL);
    ts->tv_nsec = (s64)((ns100 % 10'000'000ULL) * 100);
    return 0;
  }
  case ORBIS_CLOCK_MONOTONIC:
  case ORBIS_CLOCK_MONOTONIC_PRECISE:
  case ORBIS_CLOCK_MONOTONIC_FAST:
  case ORBIS_CLOCK_UPTIME:
  case ORBIS_CLOCK_UPTIME_PRECISE:
  case ORBIS_CLOCK_UPTIME_FAST:
  case ORBIS_CLOCK_PROCTIME: {
    static LARGE_INTEGER pf = [] {
      LARGE_INTEGER res{};
      QueryPerformanceFrequency(&res);
      return res;
    }();
    LARGE_INTEGER pc{};
    QueryPerformanceCounter(&pc);
    ts->tv_sec = pc.QuadPart / pf.QuadPart;
    ts->tv_nsec = ((pc.QuadPart % pf.QuadPart) * 1'000'000'000LL) / pf.QuadPart;
    return 0;
  }
  case ORBIS_CLOCK_THREAD_CPUTIME_ID: {
    FILETIME ct, et, kt, ut;
    GetThreadTimes(GetCurrentThread(), &ct, &et, &kt, &ut);
    u64 ns100 = ((u64)ut.dwHighDateTime << 32 | ut.dwLowDateTime) +
                ((u64)kt.dwHighDateTime << 32 | kt.dwLowDateTime);
    ts->tv_sec = (s64)(ns100 / 10'000'000ULL);
    ts->tv_nsec = (s64)((ns100 % 10'000'000ULL) * 100);
    return 0;
  }
  default:
    printf("[Time] posix_clock_gettime: unsupported clock_id %u\n", clock_id);
    return -1;
  }
#else
  // Linux/macOS fallback
  struct timespec t{};
  clock_gettime(CLOCK_MONOTONIC, &t);
  ts->tv_sec = t.tv_sec;
  ts->tv_nsec = t.tv_nsec;
  return 0;
#endif
}

s32 PS4_SYSV_ABI sceKernelClockGettime(u32 clock_id, OrbisKernelTimespec *ts) {
  s32 ret = posix_clock_gettime(clock_id, ts);
  if (ret < 0)
    return ErrnoToSceKernelError(-1);
  return 0;
}

s32 PS4_SYSV_ABI posix_nanosleep(const OrbisKernelTimespec *rqtp,
                                 OrbisKernelTimespec *rmtp) {
  if (!rqtp || rqtp->tv_sec < 0 || rqtp->tv_nsec < 0 ||
      rqtp->tv_nsec >= 1'000'000'000LL)
    return -1;

  auto dur = std::chrono::seconds(rqtp->tv_sec) +
             std::chrono::nanoseconds(rqtp->tv_nsec);
  std::this_thread::sleep_for(dur);

  if (rmtp) {
    rmtp->tv_sec = 0;
    rmtp->tv_nsec = 0;
  }
  return 0;
}

s32 PS4_SYSV_ABI sceKernelNanosleep(const OrbisKernelTimespec *rqtp,
                                    OrbisKernelTimespec *rmtp) {
  return posix_nanosleep(rqtp, rmtp);
}

s32 PS4_SYSV_ABI posix_usleep(u32 microseconds) {
  std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
  return 0;
}

s32 PS4_SYSV_ABI sceKernelUsleep(u32 microseconds) {
  return posix_usleep(microseconds);
}

u32 PS4_SYSV_ABI posix_sleep(u32 seconds) {
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
  return 0;
}

s32 PS4_SYSV_ABI sceKernelSleep(u32 seconds) {
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
  return 0;
}

s32 PS4_SYSV_ABI posix_gettimeofday(OrbisKernelTimeval *tp,
                                    OrbisKernelTimezone *tz) {
#ifdef _WIN64
  if (tp) {
    FILETIME ft;
    GetSystemTimePreciseAsFileTime(&ft);
    constexpr u64 UNIX_EPOCH_OFFSET = 116444736000000000ULL;
    u64 ticks = ((u64)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    ticks -= UNIX_EPOCH_OFFSET;
    ticks /= 10; // to microseconds
    tp->tv_sec = (s64)(ticks / 1'000'000ULL);
    tp->tv_usec = (s64)(ticks % 1'000'000ULL);
  }
  if (tz) {
    _tzset();
    tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
  }
  return 0;
#else
  struct timeval tv;
  struct timezone tzz;
  gettimeofday(&tv, &tzz);
  if (tp) {
    tp->tv_sec = tv.tv_sec;
    tp->tv_usec = tv.tv_usec;
  }
  if (tz) {
    tz->tz_minuteswest = tzz.tz_minuteswest;
    tz->tz_dsttime = tzz.tz_dsttime;
  }
  return 0;
#endif
}

s32 PS4_SYSV_ABI sceKernelGettimeofday(OrbisKernelTimeval *tp) {
  return posix_gettimeofday(tp, nullptr);
}

s32 PS4_SYSV_ABI sceKernelGettimezone(OrbisKernelTimezone *tz) {
  return posix_gettimeofday(nullptr, tz);
}

void RegisterTime(::Core::Kernel::ModuleManager *module_manager) {
  printf("[Kernel] Registering Time Syscalls\n");

#define LIB_FUNCTION(nid, library, version, module, function)                  \
  module_manager->RegisterHLEExport(module, nid, #function,                    \
                                    reinterpret_cast<uint64_t>(function));

  // POSIX time
  LIB_FUNCTION("NhpspxdjEKU", "libkernel", 1, "libkernel", posix_nanosleep);
  LIB_FUNCTION("NhpspxdjEKU", "libScePosix", 1, "libkernel", posix_nanosleep);
  LIB_FUNCTION("yS8U2TGCe1A", "libkernel", 1, "libkernel", posix_nanosleep);
  LIB_FUNCTION("yS8U2TGCe1A", "libScePosix", 1, "libkernel", posix_nanosleep);
  LIB_FUNCTION("QcteRwbsnV0", "libkernel", 1, "libkernel", posix_usleep);
  LIB_FUNCTION("QcteRwbsnV0", "libScePosix", 1, "libkernel", posix_usleep);
  LIB_FUNCTION("0wu33hunNdE", "libkernel", 1, "libkernel", posix_sleep);
  LIB_FUNCTION("0wu33hunNdE", "libScePosix", 1, "libkernel", posix_sleep);
  LIB_FUNCTION("lLMT9vJAck0", "libkernel", 1, "libkernel", posix_clock_gettime);
  LIB_FUNCTION("lLMT9vJAck0", "libScePosix", 1, "libkernel",
               posix_clock_gettime);
  LIB_FUNCTION("n88vx3C5nW8", "libkernel", 1, "libkernel", posix_gettimeofday);
  LIB_FUNCTION("n88vx3C5nW8", "libScePosix", 1, "libkernel",
               posix_gettimeofday);

  // Orbis time
  LIB_FUNCTION("4J2sUJmuHZQ", "libkernel", 1, "libkernel",
               sceKernelGetProcessTime);
  LIB_FUNCTION("fgxnMeTNUtY", "libkernel", 1, "libkernel",
               sceKernelGetProcessTimeCounter);
  LIB_FUNCTION("BNowx2l588E", "libkernel", 1, "libkernel",
               sceKernelGetProcessTimeCounterFrequency);
  LIB_FUNCTION("-2IRUCO--PM", "libkernel", 1, "libkernel", sceKernelReadTsc);
  LIB_FUNCTION("1j3S3n-tTW4", "libkernel", 1, "libkernel",
               sceKernelGetTscFrequency);
  LIB_FUNCTION("QvsZxomvUHs", "libkernel", 1, "libkernel", sceKernelNanosleep);
  LIB_FUNCTION("1jfXLRVzisc", "libkernel", 1, "libkernel", sceKernelUsleep);
  LIB_FUNCTION("-ZR+hG7aDHw", "libkernel", 1, "libkernel", sceKernelSleep);
  LIB_FUNCTION("QBi7HCK03hw", "libkernel", 1, "libkernel",
               sceKernelClockGettime);
  LIB_FUNCTION("ejekcaNQNq0", "libkernel", 1, "libkernel",
               sceKernelGettimeofday);
  LIB_FUNCTION("kOcnerypnQA", "libkernel", 1, "libkernel",
               sceKernelGettimezone);

#undef LIB_FUNCTION
}

} // namespace Kernel
} // namespace Libraries
} // namespace Core
