// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
//
// NpTrophy HLE — manages trophy contexts and tracks unlock state.

#include "nptrophy.h"
#include <cstdio>
#include <cstring>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace Core {
namespace Libraries {
namespace NpTrophy {

struct TrophyContext {
  bool active = false;
  s32 handle = -1;
  s32 userId = -1;
  std::string titleId;
  std::set<s32> unlocked_trophies;
  bool registered = false;
};

static constexpr int MAX_TROPHY_CTX = 4;
static TrophyContext g_contexts[MAX_TROPHY_CTX];
static std::mutex g_trophy_mutex;
static s32 g_next_handle = 1;

static s32 PS4_SYSV_ABI sceNpTrophyCreateContext(s32 *context,
                                                     s32 userId,
                                                     u32 serviceLabel,
                                                     u64 options) {
  std::lock_guard<std::mutex> lock(g_trophy_mutex);
  if (!context) return -1;

  printf("[NpTrophy] CreateContext: user=%d, label=%u\n", userId, serviceLabel);

  for (int i = 0; i < MAX_TROPHY_CTX; i++) {
    if (!g_contexts[i].active) {
      g_contexts[i].active = true;
      g_contexts[i].handle = g_next_handle++;
      g_contexts[i].userId = userId;
      g_contexts[i].registered = false;
      g_contexts[i].unlocked_trophies.clear();
      *context = g_contexts[i].handle;
      printf("[NpTrophy]   Created context handle=%d\n", *context);
      return 0;
    }
  }
  return -1;
}

static s32 PS4_SYSV_ABI sceNpTrophyRegisterContext(s32 context,
                                                       s32 handle,
                                                       u64 options) {
  std::lock_guard<std::mutex> lock(g_trophy_mutex);

  printf("[NpTrophy] RegisterContext: ctx=%d\n", context);

  for (int i = 0; i < MAX_TROPHY_CTX; i++) {
    if (g_contexts[i].active && g_contexts[i].handle == context) {
      g_contexts[i].registered = true;
      printf("[NpTrophy]   Context %d registered\n", context);
      return 0;
    }
  }
  return -1;
}

static s32 PS4_SYSV_ABI sceNpTrophyDestroyContext(s32 context) {
  std::lock_guard<std::mutex> lock(g_trophy_mutex);

  printf("[NpTrophy] DestroyContext: ctx=%d\n", context);

  for (int i = 0; i < MAX_TROPHY_CTX; i++) {
    if (g_contexts[i].active && g_contexts[i].handle == context) {
      g_contexts[i].active = false;
      return 0;
    }
  }
  return -1;
}

static s32 PS4_SYSV_ABI sceNpTrophyUnlockTrophy(s32 context,
                                                    s32 trophyId,
                                                    s32 *platinumId) {
  std::lock_guard<std::mutex> lock(g_trophy_mutex);

  printf("[NpTrophy] UnlockTrophy: ctx=%d, trophy=%d\n", context, trophyId);

  for (int i = 0; i < MAX_TROPHY_CTX; i++) {
    if (g_contexts[i].active && g_contexts[i].handle == context) {
      if (!g_contexts[i].registered) return -1;

      g_contexts[i].unlocked_trophies.insert(trophyId);
      printf("[NpTrophy]   Trophy %d unlocked! (total: %zu)\n",
             trophyId, g_contexts[i].unlocked_trophies.size());

      if (platinumId) *platinumId = -1; // No platinum check yet
      return 0;
    }
  }
  return -1;
}

static s32 PS4_SYSV_ABI sceNpTrophyGetTrophyUnlockState(
    s32 context, void *flags, u32 *count) {
  std::lock_guard<std::mutex> lock(g_trophy_mutex);

  for (int i = 0; i < MAX_TROPHY_CTX; i++) {
    if (g_contexts[i].active && g_contexts[i].handle == context) {
      u32 total = static_cast<u32>(g_contexts[i].unlocked_trophies.size());
      if (count) *count = total;

      // Flags is a bitfield — set bit N if trophy N is unlocked
      if (flags) {
        memset(flags, 0, 128); // 1024 bits = 128 bytes max
        auto *bits = static_cast<u8*>(flags);
        for (s32 tid : g_contexts[i].unlocked_trophies) {
          if (tid >= 0 && tid < 1024) {
            bits[tid / 8] |= (1 << (tid % 8));
          }
        }
      }
      return 0;
    }
  }
  return -1;
}

static s32 PS4_SYSV_ABI sceNpTrophyGetGameInfo(s32 context,
                                                   void *details,
                                                   void *data) {
  // Fill with minimal info: game has trophies
  if (details) {
    memset(details, 0, 256);
  }
  if (data) {
    memset(data, 0, 256);
  }
  return 0;
}

void RegisterNpTrophy(::Core::Kernel::ModuleManager *module_manager) {
  printf("[NpTrophy] Registering NpTrophy\n");

#define LIB_FUNCTION(nid, library, version, module, function)                  \
  module_manager->RegisterHLEExport(module, nid, #function,                    \
                                    reinterpret_cast<uint64_t>(function));

  LIB_FUNCTION("TJCAxto9SEE", "libSceNpTrophy", 1, "libSceNpTrophy",
               sceNpTrophyCreateContext);
  LIB_FUNCTION("W3rZ+K1wVks", "libSceNpTrophy", 1, "libSceNpTrophy",
               sceNpTrophyRegisterContext);
  LIB_FUNCTION("a8fU1W0f2o4", "libSceNpTrophy", 1, "libSceNpTrophy",
               sceNpTrophyDestroyContext);
  LIB_FUNCTION("wL1aA8nB1xM", "libSceNpTrophy", 1, "libSceNpTrophy",
               sceNpTrophyUnlockTrophy);
  LIB_FUNCTION("7HJxV3ob1Rc", "libSceNpTrophy", 1, "libSceNpTrophy",
               sceNpTrophyGetTrophyUnlockState);
  LIB_FUNCTION("GNcF4oidY0Y", "libSceNpTrophy", 1, "libSceNpTrophy",
               sceNpTrophyGetGameInfo);

#undef LIB_FUNCTION
}

} // namespace NpTrophy
} // namespace Libraries
} // namespace Core
