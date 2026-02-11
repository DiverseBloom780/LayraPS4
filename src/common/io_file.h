#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace Common {
namespace IO {

enum class FileType { BinaryFile, TextFile };

class File {
public:
  File(const std::string &path);
  ~File();
  bool Open(const std::string &mode);
  void Close();
  size_t Read(void *buffer, size_t size);
  size_t Write(const void *buffer, size_t size);
  bool IsOpen() const;
  size_t GetSize() const;

private:
  std::string path;
  void *handle;
  bool is_open;
};

enum class FileAccessMode {
  Read,
  Write,
  Append,
  ReadWrite,
  ReadAppend,
  Create
};
} // namespace IO
} // namespace Common
