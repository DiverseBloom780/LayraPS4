#pragma once
#include <cassert>
#include <iostream>

#ifdef NDEBUG
#define ASSERT(condition) ((void)0)
#define ASSERT_MSG(condition, message) ((void)0)
#define UNREACHABLE() __builtin_unreachable()
#else
#define ASSERT(condition) assert(condition)
#define ASSERT_MSG(condition, message)                                         \
  if (!(condition)) {                                                          \
    std::cerr << "Assertion failed: " << #condition << ", " << message         \
              << "\n";                                                         \
    std::abort();                                                              \
  }
#define UNREACHABLE()                                                          \
  std::cerr << "Unreachable code hit!\n";                                      \
  std::abort()

#endif

#include <format>
#define UNREACHABLEMSG(...)                                                    \
  do {                                                                         \
    std::cerr << "Unreachable code hit: " << std::format(__VA_ARGS__) << "\n"; \
    std::abort();                                                              \
  } while (0)
