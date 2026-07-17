// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/io_file.h"
#include <cerrno>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifdef _MSC_VER
#define fseeko _fseeki64
#define ftello _ftelli64
#endif

namespace Common {
namespace IO {

namespace {

#ifdef _WIN32
[[nodiscard]] constexpr const wchar_t *AccessModeToWStr(FileAccessMode mode) {
  switch (mode) {
  case FileAccessMode::Read:
    return L"rb";
  case FileAccessMode::Write:
    return L"wb";
  case FileAccessMode::Append:
    return L"ab";
  case FileAccessMode::ReadWrite:
    return L"r+b";
  case FileAccessMode::ReadAppend:
    return L"a+b";
  case FileAccessMode::Create:
    return L"w+b";
  default:
    return L"rb";
  }
}
#else
[[nodiscard]] constexpr const char *AccessModeToStr(FileAccessMode mode) {
  switch (mode) {
  case FileAccessMode::Read:
    return "rb";
  case FileAccessMode::Write:
    return "wb";
  case FileAccessMode::Append:
    return "ab";
  case FileAccessMode::ReadWrite:
    return "r+b";
  case FileAccessMode::ReadAppend:
    return "a+b";
  case FileAccessMode::Create:
    return "w+b";
  default:
    return "rb";
  }
}
#endif

} // namespace

File::File() = default;

File::File(const std::filesystem::path &path, FileAccessMode mode) {
  Open(path, mode);
}

File::~File() { Close(); }

File::File(File &&other) noexcept
    : path_(std::move(other.path_)), file_(other.file_) {
  other.file_ = nullptr;
}

File &File::operator=(File &&other) noexcept {
  if (this != &other) {
    Close();
    path_ = std::move(other.path_);
    file_ = other.file_;
    other.file_ = nullptr;
  }
  return *this;
}

int File::Open(const std::filesystem::path &path, FileAccessMode mode) {
  Close();
  path_ = path;

#ifdef _WIN32
  const auto wpath = path.wstring();
  const auto wmode = AccessModeToWStr(mode);
  errno_t err = _wfopen_s(&file_, wpath.c_str(), wmode);
  if (err != 0) {
    file_ = nullptr;
    return err;
  }
#else
  const auto spath = path.string();
  const auto smode = AccessModeToStr(mode);
  file_ = std::fopen(spath.c_str(), smode);
  if (!file_) {
    return errno;
  }
#endif

  return 0;
}

void File::Close() {
  if (file_) {
    std::fclose(file_);
    file_ = nullptr;
  }
}

size_t File::Read(void *buffer, size_t size) {
  if (!file_ || !buffer || size == 0) {
    return 0;
  }
  return std::fread(buffer, 1, size, file_);
}

size_t File::Write(const void *buffer, size_t size) {
  if (!file_ || !buffer || size == 0) {
    return 0;
  }
  return std::fwrite(buffer, 1, size, file_);
}

int64_t File::Seek(int64_t offset, int whence) {
  if (!file_) {
    return -1;
  }
  if (fseeko(file_, offset, whence) != 0) {
    return -1;
  }
  return ftello(file_);
}

int64_t File::Tell() const {
  if (!file_) {
    return -1;
  }
  return ftello(file_);
}

size_t File::GetSize() const {
  if (!file_) {
    return 0;
  }
  // Save current position
  int64_t current = ftello(file_);
  if (current < 0) {
    return 0;
  }
  // Seek to end
  fseeko(file_, 0, SEEK_END);
  int64_t size = ftello(file_);
  // Restore position
  fseeko(file_, current, SEEK_SET);
  return size >= 0 ? static_cast<size_t>(size) : 0;
}

bool File::SetSize(size_t size) {
  if (!file_) {
    return false;
  }
#ifdef _WIN32
  int fd = _fileno(file_);
  return _chsize_s(fd, static_cast<long long>(size)) == 0;
#else
  int fd = fileno(file_);
  return ftruncate(fd, static_cast<off_t>(size)) == 0;
#endif
}

bool File::IsOpen() const { return file_ != nullptr; }

} // namespace IO
} // namespace Common
