// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/io_file.h"
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>


namespace Core {
namespace FileSys {

class MntPoints {
public:
  struct MntPair {
    std::filesystem::path host_path;
    std::string mount; // e.g /app0
    bool read_only;
  };

  explicit MntPoints() = default;
  ~MntPoints() = default;

  void Mount(const std::filesystem::path &host_folder,
             const std::string &guest_folder, bool read_only = false);
  void Unmount(const std::string &guest_folder);
  void UnmountAll();

  std::filesystem::path GetHostPath(std::string_view guest_path,
                                    bool *is_read_only = nullptr);

private:
  std::vector<MntPair> m_mnt_pairs;
  std::mutex m_mutex;
};

enum class FileType { Regular, Directory, Device, Socket };

struct File {
  std::atomic_bool is_opened{false};
  std::atomic<FileType> type{FileType::Regular};
  std::filesystem::path m_host_name;
  std::string m_guest_name;
  std::unique_ptr<Common::IO::File> f;
  std::mutex m_mutex;

  File() = default;
};

class HandleTable {
public:
  HandleTable() = default;
  ~HandleTable();

  int CreateHandle();
  void DeleteHandle(int fd);
  File *GetFile(int fd);

  void CreateStdHandles();

private:
  std::vector<std::unique_ptr<File>> m_files;
  std::mutex m_mutex;
};

} // namespace FileSys
} // namespace Core
