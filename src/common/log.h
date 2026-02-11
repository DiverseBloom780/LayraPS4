// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <chrono>
#include <format>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace Log {

enum class Level { Trace, Debug, Info, Warning, Error, Critical };

class Logger {
public:
  static Logger &Instance() {
    static Logger instance;
    return instance;
  }

  void Log(Level level, const std::string &category,
           const std::string &message) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::cout << "["
              << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
              << "] "
              << "[" << LevelToString(level) << "] "
              << "[" << category << "] " << message << std::endl;
  }

private:
  std::string LevelToString(Level level) {
    switch (level) {
    case Level::Trace:
      return "TRACE";
    case Level::Debug:
      return "DEBUG";
    case Level::Info:
      return "INFO";
    case Level::Warning:
      return "WARN";
    case Level::Error:
      return "ERROR";
    case Level::Critical:
      return "CRITICAL";
    default:
      return "UNKNOWN";
    }
  }

  std::mutex mutex_;
};

inline void Log(Level level, const std::string &category,
                const std::string &message) {
  Logger::Instance().Log(level, category, message);
}

} // namespace Log

#define LOG_TRACE(category, ...)                                               \
  Log::Log(Log::Level::Trace, category, std::format(__VA_ARGS__))
#define LOG_DEBUG(category, ...)                                               \
  Log::Log(Log::Level::Debug, category, std::format(__VA_ARGS__))
#define LOG_INFO(category, ...)                                                \
  Log::Log(Log::Level::Info, category, std::format(__VA_ARGS__))
#define LOG_WARNING(category, ...)                                             \
  Log::Log(Log::Level::Warning, category, std::format(__VA_ARGS__))
#define LOG_ERROR(category, ...)                                               \
  Log::Log(Log::Level::Error, category, std::format(__VA_ARGS__))
#define LOG_CRITICAL(category, ...)                                            \
  Log::Log(Log::Level::Critical, category, std::format(__VA_ARGS__))