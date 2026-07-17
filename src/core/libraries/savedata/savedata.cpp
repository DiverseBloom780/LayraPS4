// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
//
// SaveData HLE — manages game save directories on the host filesystem.

#include "savedata.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace Core {
namespace Libraries {
namespace SaveData {

// PS4 save data structures
struct OrbisSaveDataMount {
  s32 userId;
  char titleId[16];
  char dirName[32];
  char fingerprint[65];
  u32 blocks;
  u32 mountMode;
};

struct OrbisSaveDataMountResult {
  char mountPoint[16];
  u64 requiredBlocks;
  u32 progress;
};

// Internal mount tracking
struct MountEntry {
  bool active = false;
  std::string host_path;      // Actual host directory
  std::string mount_point;    // e.g. "/savedata0"
  std::string title_id;
  std::string dir_name;
};

static constexpr int MAX_MOUNTS = 16;
static MountEntry g_mounts[MAX_MOUNTS];
static bool g_initialized = false;
static std::mutex g_save_mutex;
static int g_mount_counter = 0;

// Base directory for all save data on host
static std::string GetSaveBasePath() {
  // Store saves next to the executable in a /savedata/ folder
  return "savedata";
}

static s32 PS4_SYSV_ABI sceSaveDataInitialize3(void *initParam) {
  std::lock_guard<std::mutex> lock(g_save_mutex);
  if (g_initialized) return 0;

  printf("[SaveData] sceSaveDataInitialize3\n");

  // Create base save directory if it doesn't exist
  std::filesystem::create_directories(GetSaveBasePath());
  g_initialized = true;
  return 0;
}

static s32 PS4_SYSV_ABI sceSaveDataMount(const OrbisSaveDataMount *mount,
                                           OrbisSaveDataMountResult *result) {
  std::lock_guard<std::mutex> lock(g_save_mutex);

  if (!mount || !result) return -1;

  printf("[SaveData] sceSaveDataMount: user=%d, title='%s', dir='%s', "
         "mode=%u\n",
         mount->userId, mount->titleId, mount->dirName, mount->mountMode);

  // Build host path: savedata/<titleId>/<dirName>
  std::string hostPath = GetSaveBasePath() + "/" +
                          std::string(mount->titleId) + "/" +
                          std::string(mount->dirName);

  // Create directory if it doesn't exist (for write modes)
  bool isWriteMode = (mount->mountMode & 0x2) != 0; // CREATE or RDWR
  if (isWriteMode) {
    std::filesystem::create_directories(hostPath);
  }

  // Check if the directory exists (for read modes)
  bool exists = std::filesystem::exists(hostPath);
  if (!exists && !isWriteMode) {
    printf("[SaveData]   Directory does not exist: %s\n", hostPath.c_str());
    return 0x809F0008; // SCE_SAVE_DATA_ERROR_NOT_FOUND
  }

  // Find a free mount slot
  for (int i = 0; i < MAX_MOUNTS; i++) {
    if (!g_mounts[i].active) {
      g_mounts[i].active = true;
      g_mounts[i].host_path = hostPath;
      g_mounts[i].title_id = mount->titleId;
      g_mounts[i].dir_name = mount->dirName;

      // Generate mount point name
      char mp[16];
      snprintf(mp, sizeof(mp), "/savedata%d", g_mount_counter++);
      g_mounts[i].mount_point = mp;

      // Fill result
      memset(result, 0, sizeof(OrbisSaveDataMountResult));
      strncpy(result->mountPoint, mp, sizeof(result->mountPoint) - 1);
      result->requiredBlocks = 0;
      result->progress = 100;

      printf("[SaveData]   Mounted at '%s' -> %s\n", mp, hostPath.c_str());
      return 0;
    }
  }

  fprintf(stderr, "[SaveData] ERROR: No free mount slots!\n");
  return -1;
}

static s32 PS4_SYSV_ABI sceSaveDataUmount(const char *mountPoint) {
  std::lock_guard<std::mutex> lock(g_save_mutex);

  if (!mountPoint) return -1;

  printf("[SaveData] sceSaveDataUmount: '%s'\n", mountPoint);

  for (int i = 0; i < MAX_MOUNTS; i++) {
    if (g_mounts[i].active && g_mounts[i].mount_point == mountPoint) {
      g_mounts[i].active = false;
      return 0;
    }
  }
  return -1; // Not found
}

static s32 PS4_SYSV_ABI sceSaveDataDelete(s32 userId,
                                             const char *titleId,
                                             const char *dirName) {
  printf("[SaveData] sceSaveDataDelete: user=%d, title='%s', dir='%s'\n",
         userId, titleId ? titleId : "null", dirName ? dirName : "null");

  if (!titleId || !dirName) return -1;

  std::string hostPath = GetSaveBasePath() + "/" +
                          std::string(titleId) + "/" +
                          std::string(dirName);

  if (std::filesystem::exists(hostPath)) {
    std::filesystem::remove_all(hostPath);
    printf("[SaveData]   Deleted: %s\n", hostPath.c_str());
    return 0;
  }
  return 0x809F0008; // NOT_FOUND
}

static s32 PS4_SYSV_ABI sceSaveDataGetMountInfo(const char *mountPoint,
                                                   void *info) {
  if (!mountPoint || !info) return -1;

  std::lock_guard<std::mutex> lock(g_save_mutex);
  for (int i = 0; i < MAX_MOUNTS; i++) {
    if (g_mounts[i].active && g_mounts[i].mount_point == mountPoint) {
      // Info struct: fill with basic data
      memset(info, 0, 64);
      // blocks = directory size / 32768
      auto dirSize = std::filesystem::exists(g_mounts[i].host_path)
                         ? 1024ULL * 1024 // estimate 1MB
                         : 0ULL;
      u64 blocks = dirSize / 32768;
      memcpy(info, &blocks, sizeof(u64));
      return 0;
    }
  }
  return -1;
}

void RegisterSaveData(::Core::Kernel::ModuleManager *module_manager) {
  printf("[SaveData] Registering SaveData\n");

#define LIB_FUNCTION(nid, library, version, module, function)                  \
  module_manager->RegisterHLEExport(module, nid, #function,                    \
                                    reinterpret_cast<uint64_t>(function));

  LIB_FUNCTION("vbDmhvyS0Vo", "libSceSaveData", 1, "libSceSaveData",
               sceSaveDataInitialize3);
  LIB_FUNCTION("32HwGGKFKjg", "libSceSaveData", 1, "libSceSaveData",
               sceSaveDataMount);
  LIB_FUNCTION("VHRXkMkTFJo", "libSceSaveData", 1, "libSceSaveData",
               sceSaveDataUmount);
  LIB_FUNCTION("wU2dwEGOKOA", "libSceSaveData", 1, "libSceSaveData",
               sceSaveDataDelete);
  LIB_FUNCTION("p1bHJjLdVuk", "libSceSaveData", 1, "libSceSaveData",
               sceSaveDataGetMountInfo);

#undef LIB_FUNCTION
}

} // namespace SaveData
} // namespace Libraries
} // namespace Core
