// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "equeue.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <map>
#include <mutex>

namespace Core {
namespace Libraries {
namespace Kernel {

// --- EqueueInternal ---

bool EqueueInternal::AddEvent(const EqueueEvent &ev) {
  std::lock_guard<std::mutex> lock(m_mutex);
  // Update existing or add new
  for (auto &e : m_events) {
    if (e.event.ident == ev.event.ident && e.event.filter == ev.event.filter) {
      e = ev;
      return true;
    }
  }
  m_events.push_back(ev);
  return true;
}

bool EqueueInternal::RemoveEvent(u64 id, s16 filter) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = std::remove_if(
      m_events.begin(), m_events.end(), [id, filter](const EqueueEvent &e) {
        return e.event.ident == id && e.event.filter == filter;
      });
  if (it != m_events.end()) {
    m_events.erase(it, m_events.end());
    return true;
  }
  return false;
}

bool EqueueInternal::TriggerEvent(u64 ident, s16 filter, void *trigger_data) {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto &e : m_events) {
    if (e.event.ident == ident && e.event.filter == filter) {
      e.is_triggered = true;
      e.event.data = reinterpret_cast<u64>(trigger_data);
      m_cond.notify_all();
      return true;
    }
  }
  return false;
}

int EqueueInternal::GetTriggeredEvents(OrbisKernelEvent *ev, int num) {
  int count = 0;
  for (auto &e : m_events) {
    if (e.is_triggered && count < num) {
      ev[count++] = e.event;
      e.is_triggered = false;
      e.event.data = 0;
    }
  }
  return count;
}

int EqueueInternal::WaitForEvents(OrbisKernelEvent *ev, int num,
                                  u32 *timeout_us) {
  std::unique_lock<std::mutex> lock(m_mutex);

  // Check for already triggered events first
  int count = GetTriggeredEvents(ev, num);
  if (count > 0)
    return count;

  if (timeout_us && *timeout_us == 0) {
    return 0; // Poll mode, no wait
  }

  // Wait with timeout
  auto pred = [this]() {
    for (const auto &e : m_events) {
      if (e.is_triggered)
        return true;
    }
    return false;
  };

  if (timeout_us) {
    auto dur = std::chrono::microseconds(*timeout_us);
    m_cond.wait_for(lock, dur, pred);
  } else {
    // Indefinite wait - use a reasonable max to avoid true deadlocks
    m_cond.wait_for(lock, std::chrono::seconds(30), pred);
  }

  return GetTriggeredEvents(ev, num);
}

// --- Global EQueue Registry ---

static std::map<OrbisKernelEqueue, EqueueInternal *> g_equeues;
static std::mutex g_eq_mutex;
static OrbisKernelEqueue g_next_eq = 0x100;

s32 PS4_SYSV_ABI sceKernelCreateEqueue(OrbisKernelEqueue *eq,
                                       const char *name) {
  if (!eq)
    return -1;

  std::lock_guard<std::mutex> lock(g_eq_mutex);
  OrbisKernelEqueue handle = g_next_eq++;
  auto *internal = new EqueueInternal(handle, name ? name : "unnamed");
  g_equeues[handle] = internal;
  *eq = handle;

  printf("[Kernel] sceKernelCreateEqueue: '%s' -> handle=0x%llx\n",
         name ? name : "unnamed", (unsigned long long)handle);
  return 0;
}

s32 PS4_SYSV_ABI sceKernelDeleteEqueue(OrbisKernelEqueue eq) {
  std::lock_guard<std::mutex> lock(g_eq_mutex);
  auto it = g_equeues.find(eq);
  if (it != g_equeues.end()) {
    delete it->second;
    g_equeues.erase(it);
    return 0;
  }
  return -1;
}

static EqueueInternal *GetEqueue(OrbisKernelEqueue eq) {
  std::lock_guard<std::mutex> lock(g_eq_mutex);
  auto it = g_equeues.find(eq);
  return it != g_equeues.end() ? it->second : nullptr;
}

s32 PS4_SYSV_ABI sceKernelAddUserEvent(OrbisKernelEqueue eq, s32 id) {
  auto *internal = GetEqueue(eq);
  if (!internal)
    return -1;

  EqueueEvent ev{};
  ev.event.ident = id;
  ev.event.filter = EVFILT_USER;
  ev.event.udata = nullptr;
  internal->AddEvent(ev);
  return 0;
}

s32 PS4_SYSV_ABI sceKernelAddUserEventEdge(OrbisKernelEqueue eq, s32 id) {
  return sceKernelAddUserEvent(eq, id);
}

s32 PS4_SYSV_ABI sceKernelWaitEqueue(OrbisKernelEqueue eq, OrbisKernelEvent *ev,
                                     s32 num, s32 *out, u32 *timo) {
  if (!ev || num < 1)
    return -1;

  auto *internal = GetEqueue(eq);
  if (!internal)
    return -1;

  int count = internal->WaitForEvents(ev, num, timo);
  if (out)
    *out = count;
  return 0;
}

void RegisterEventQueue(::Core::Kernel::ModuleManager *module_manager) {
  printf("[Kernel] Registering Event Queue Syscalls\n");

#define LIB_FUNCTION(nid, library, version, module, function)                  \
  module_manager->RegisterHLEExport(module, nid, #function,                    \
                                    reinterpret_cast<uint64_t>(function));

  LIB_FUNCTION("D0OdFMjp46I", "libkernel", 1, "libkernel",
               sceKernelCreateEqueue);
  LIB_FUNCTION("jpFjmgAC5AE", "libkernel", 1, "libkernel",
               sceKernelDeleteEqueue);
  LIB_FUNCTION("fzyMKs9kim0", "libkernel", 1, "libkernel", sceKernelWaitEqueue);
  LIB_FUNCTION("vz+pg2zdDpo", "libkernel", 1, "libkernel",
               sceKernelAddUserEvent);
  LIB_FUNCTION("4R6-OvI2cEA", "libkernel", 1, "libkernel",
               sceKernelAddUserEventEdge);

#undef LIB_FUNCTION
}

} // namespace Kernel
} // namespace Libraries
} // namespace Core
