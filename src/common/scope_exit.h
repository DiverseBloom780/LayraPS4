#pragma once
#include <functional>

namespace Common {

class ScopeExit {
public:
  explicit ScopeExit(std::function<void()> exit_func) : exit_func_(exit_func) {}
  ~ScopeExit() {
    if (exit_func_) {
      exit_func_();
    }
  }

  ScopeExit(const ScopeExit &) = delete;
  ScopeExit &operator=(const ScopeExit &) = delete;
  ScopeExit(ScopeExit &&) = default;
  ScopeExit &operator=(ScopeExit &&) = default;

private:
  std::function<void()> exit_func_;
};

} // namespace Common

#define SCOPE_EXIT(func) Common::ScopeExit scope_exit_##__LINE__(func)
