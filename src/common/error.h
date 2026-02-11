#pragma once
#include <string>

namespace Common {
class Error {
public:
  Error(const std::string &msg) : message(msg) {}
  const char *what() const { return message.c_str(); }

private:
  std::string message;
};
} // namespace Common
