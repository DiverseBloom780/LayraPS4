// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "fs.h"
#include <algorithm>
#include <cstdio>
#include <mutex>
#include <string_view>

namespace Core {
namespace FileSys {

// MntPoints Implementation
void MntPoints::Mount(const std::filesystem::path &host_folder,
                      const std::string &guest_folder, bool read_only) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_mnt_pairs.push_back({host_folder, guest_folder, read_only});
  printf("[FileSystem] Mounted %s -> %ls (RO: %d)\n", guest_folder.c_str(),
         host_folder.c_str(), read_only);
}

void MntPoints::Unmount(const std::string &guest_folder) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_mnt_pairs.erase(std::remove_if(m_mnt_pairs.begin(), m_mnt_pairs.end(),
                                   [&](const MntPair &pair) {
                                     return pair.mount == guest_folder;
                                   }),
                    m_mnt_pairs.end());
}

void MntPoints::UnmountAll() {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_mnt_pairs.clear();
}

std::filesystem::path MntPoints::GetHostPath(std::string_view guest_path,
                                             bool *is_read_only) {
  std::lock_guard<std::mutex> lock(m_mutex);

  const MntPair *best_match = nullptr;
  size_t longest_match = 0;

  for (const auto &pair : m_mnt_pairs) {
    if (guest_path.starts_with(pair.mount)) {
      if (pair.mount.length() > longest_match) {
        longest_match = pair.mount.length();
        best_match = &pair;
      }
    }
  }

  if (best_match) {
    if (is_read_only)
      *is_read_only = best_match->read_only;

    std::string relative_path = std::string(guest_path.substr(longest_match));
    if (!relative_path.empty() &&
        (relative_path[0] == '/' || relative_path[0] == '\\')) {
      relative_path.erase(0, 1);
    }

    return best_match->host_path / relative_path;
  }

  return std::filesystem::path();
}

// HandleTable Implementation
HandleTable::~HandleTable() {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_files.clear();
}

int HandleTable::CreateHandle() {
  std::lock_guard<std::mutex> lock(m_mutex);

  for (size_t i = 0; i < m_files.size(); ++i) {
    if (!m_files[i]) {
      m_files[i] = std::make_unique<File>();
      return static_cast<int>(i);
    }
  }

  m_files.push_back(std::make_unique<File>());
  return static_cast<int>(m_files.size() - 1);
}

void HandleTable::DeleteHandle(int fd) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (fd >= 0 && fd < static_cast<int>(m_files.size())) {
    m_files[fd].reset();
  }
}

File *HandleTable::GetFile(int fd) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (fd >= 0 && fd < static_cast<int>(m_files.size())) {
    return m_files[fd].get();
  }
  return nullptr;
}

void HandleTable::CreateStdHandles() {
  for (int i = 0; i < 3; ++i) {
    int fd = CreateHandle();
    File *file = GetFile(fd);
    if (file) {
      file->is_opened = true;
      file->type = FileType::Device;
      file->m_guest_name = (i == 0)   ? "/dev/stdin"
                           : (i == 1) ? "/dev/stdout"
                                      : "/dev/stderr";
    }
  }
}

} // namespace FileSys
} // namespace Core
