// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/kernel/module_manager.h"
#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

namespace Core {
namespace Libraries {
namespace Kernel {

struct OrbisKernelEvent {
  u64 ident = 0;
  s16 filter = 0;
  u16 flags = 0;
  u32 fflags = 0;
  u64 data = 0;
  void *udata = nullptr;
};

// Filter types
constexpr s16 EVFILT_READ = -1;
constexpr s16 EVFILT_WRITE = -2;
constexpr s16 EVFILT_AIO = -3;
constexpr s16 EVFILT_TIMER = -7;
constexpr s16 EVFILT_USER = -11;
constexpr s16 EVFILT_VIDEO_OUT = -13;
constexpr s16 EVFILT_GRAPHICS_CORE = -14;
constexpr s16 EVFILT_HRTIMER = -15;

// Flags
constexpr u16 EV_ADD = 1;
constexpr u16 EV_DELETE = 2;
constexpr u16 EV_ENABLE = 4;
constexpr u16 EV_DISABLE = 8;
constexpr u16 EV_ONESHOT = 0x10;
constexpr u16 EV_CLEAR = 0x20;

using OrbisKernelEqueue = s64;

struct EqueueEvent {
  OrbisKernelEvent event;
  bool is_triggered = false;
};

class EqueueInternal {
public:
  explicit EqueueInternal(OrbisKernelEqueue handle, const std::string &name)
      : m_handle(handle), m_name(name) {}

  bool AddEvent(const EqueueEvent &ev);
  bool RemoveEvent(u64 id, s16 filter);
  bool TriggerEvent(u64 ident, s16 filter, void *trigger_data);
  int WaitForEvents(OrbisKernelEvent *ev, int num, u32 *timeout_us);
  int GetTriggeredEvents(OrbisKernelEvent *ev, int num);

  const std::string &GetName() const { return m_name; }

private:
  OrbisKernelEqueue m_handle;
  std::string m_name;
  std::mutex m_mutex;
  std::condition_variable m_cond;
  std::vector<EqueueEvent> m_events;
};

// Syscalls
s32 PS4_SYSV_ABI sceKernelCreateEqueue(OrbisKernelEqueue *eq, const char *name);
s32 PS4_SYSV_ABI sceKernelDeleteEqueue(OrbisKernelEqueue eq);
s32 PS4_SYSV_ABI sceKernelAddUserEvent(OrbisKernelEqueue eq, s32 id);
s32 PS4_SYSV_ABI sceKernelAddUserEventEdge(OrbisKernelEqueue eq, s32 id);
s32 PS4_SYSV_ABI sceKernelWaitEqueue(OrbisKernelEqueue eq, OrbisKernelEvent *ev,
                                     s32 num, s32 *out, u32 *timo);

void RegisterEventQueue(::Core::Kernel::ModuleManager *module_manager);

} // namespace Kernel
} // namespace Libraries
} // namespace Core
