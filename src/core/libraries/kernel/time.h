// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/kernel/module_manager.h"

namespace Core {
namespace Libraries {
namespace Kernel {

struct OrbisKernelTimespec {
  s64 tv_sec;
  s64 tv_nsec;
};

struct OrbisKernelTimeval {
  s64 tv_sec;
  s64 tv_usec;
};

struct OrbisKernelTimezone {
  s32 tz_minuteswest;
  s32 tz_dsttime;
};

// Clock IDs
constexpr u32 ORBIS_CLOCK_REALTIME = 0;
constexpr u32 ORBIS_CLOCK_VIRTUAL = 1;
constexpr u32 ORBIS_CLOCK_PROF = 2;
constexpr u32 ORBIS_CLOCK_MONOTONIC = 4;
constexpr u32 ORBIS_CLOCK_UPTIME = 5;
constexpr u32 ORBIS_CLOCK_UPTIME_PRECISE = 7;
constexpr u32 ORBIS_CLOCK_UPTIME_FAST = 8;
constexpr u32 ORBIS_CLOCK_REALTIME_PRECISE = 9;
constexpr u32 ORBIS_CLOCK_REALTIME_FAST = 10;
constexpr u32 ORBIS_CLOCK_MONOTONIC_PRECISE = 11;
constexpr u32 ORBIS_CLOCK_MONOTONIC_FAST = 12;
constexpr u32 ORBIS_CLOCK_SECOND = 13;
constexpr u32 ORBIS_CLOCK_THREAD_CPUTIME_ID = 14;
constexpr u32 ORBIS_CLOCK_PROCTIME = 15;

// Orbis Time API
u64 PS4_SYSV_ABI sceKernelGetTscFrequency();
u64 PS4_SYSV_ABI sceKernelGetProcessTime();
u64 PS4_SYSV_ABI sceKernelGetProcessTimeCounter();
u64 PS4_SYSV_ABI sceKernelGetProcessTimeCounterFrequency();
u64 PS4_SYSV_ABI sceKernelReadTsc();

s32 PS4_SYSV_ABI sceKernelClockGettime(u32 clock_id, OrbisKernelTimespec *ts);
s32 PS4_SYSV_ABI sceKernelGettimeofday(OrbisKernelTimeval *tp);
s32 PS4_SYSV_ABI sceKernelGettimezone(OrbisKernelTimezone *tz);
s32 PS4_SYSV_ABI sceKernelUsleep(u32 microseconds);
s32 PS4_SYSV_ABI sceKernelNanosleep(const OrbisKernelTimespec *rqtp,
                                    OrbisKernelTimespec *rmtp);
s32 PS4_SYSV_ABI sceKernelSleep(u32 seconds);

// POSIX wrappers
s32 PS4_SYSV_ABI posix_clock_gettime(u32 clock_id, OrbisKernelTimespec *ts);
s32 PS4_SYSV_ABI posix_gettimeofday(OrbisKernelTimeval *tp,
                                    OrbisKernelTimezone *tz);
s32 PS4_SYSV_ABI posix_nanosleep(const OrbisKernelTimespec *rqtp,
                                 OrbisKernelTimespec *rmtp);
s32 PS4_SYSV_ABI posix_usleep(u32 microseconds);
u32 PS4_SYSV_ABI posix_sleep(u32 seconds);

void RegisterTime(::Core::Kernel::ModuleManager *module_manager);

} // namespace Kernel
} // namespace Libraries
} // namespace Core
