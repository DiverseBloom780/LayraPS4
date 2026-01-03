// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>

namespace Log {
    void init();
    void shutdown();
    void log(const std::string& message);
}

// Minimal logging macros for now
#define LOG_INFO(category, ...)    Log::log("INFO: " + std::string(__VA_ARGS__))
#define LOG_CRITICAL(category, ...) Log::log("CRITICAL: " + std::string(__VA_ARGS__))
#define LOG_ERROR(category, ...)   Log::log("ERROR: " + std::string(__VA_ARGS__))
#define LOG_WARN(category, ...)    Log::log("WARN: " + std::string(__VA_ARGS__))