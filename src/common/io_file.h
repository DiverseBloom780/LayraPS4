// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>

namespace Common {
namespace IO {

enum class FileType { BinaryFile, TextFile };

enum class FileAccessMode {
  Read,
  Write,
  Append,
  ReadWrite,
  ReadAppend,
  Create
};

class File {
public:
  File();
  explicit File(const std::filesystem::path &path,
                FileAccessMode mode = FileAccessMode::Read);
  ~File();

  // Non-copyable
  File(const File &) = delete;
  File &operator=(const File &) = delete;

  // Moveable
  File(File &&other) noexcept;
  File &operator=(File &&other) noexcept;

  // Open a file. Returns 0 on success, errno on failure.
  int Open(const std::filesystem::path &path, FileAccessMode mode);

  // Close the file
  void Close();

  // Read data from the file. Returns bytes read.
  size_t Read(void *buffer, size_t size);

  // Write data to the file. Returns bytes written.
  size_t Write(const void *buffer, size_t size);

  // Seek to a position. whence: 0=SET, 1=CUR, 2=END. Returns new position or -1.
  int64_t Seek(int64_t offset, int whence);

  // Get current file position.
  int64_t Tell() const;

  // Get file size.
  size_t GetSize() const;

  // Set file size (truncate or extend).
  bool SetSize(size_t size);

  // Check if the file is open.
  bool IsOpen() const;

  // Get the file path.
  const std::filesystem::path &GetPath() const { return path_; }

private:
  std::filesystem::path path_;
  FILE *file_ = nullptr;
};

} // namespace IO
} // namespace Common
