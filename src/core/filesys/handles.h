#pragma once

#include "common/types.h"
#include <map>
#include <memory>
#include <string>


namespace Libraries::Net {
struct Socket;
}

namespace Core::FileSys {

enum class FileType { None, File, Directory, Socket };

struct FileHandle {
  bool is_opened = false;
  FileType type = FileType::None;
  std::string mguestname = "unknown";
  std::shared_ptr<Libraries::Net::Socket> socket;
};

class HandleTable {
public:
  HandleTable() = default;

  s32 CreateHandle() {
    static s32 next_fd = 100;
    s32 fd = next_fd++;
    handles_[fd] = std::make_shared<FileHandle>();
    return fd;
  }

  std::shared_ptr<FileHandle> GetFile(s32 fd) {
    auto it = handles_.find(fd);
    if (it != handles_.end())
      return it->second;
    return nullptr;
  }

  std::shared_ptr<FileHandle> GetSocket(s32 fd) {
    auto f = GetFile(fd);
    if (f && f->type == FileType::Socket)
      return f;
    return nullptr;
  }

private:
  std::map<s32, std::shared_ptr<FileHandle>> handles_;
};

} // namespace Core::FileSys
