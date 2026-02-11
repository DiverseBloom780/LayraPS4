// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "common/log.h"
#include "core/networking/networking.h"
#include "types.h"
#include <filesystem>
#include <iostream>
#include <string>

namespace Config {
enum class UsbBackendType : int {
  Real,
  SkylandersPortal,
  InfinityBase,
  DimensionsToypad
};

std::string GetFoolproofInputConfigFile();
void load(const std::string &path);
Core::Networking::Config getNetworkingConfig();
int getUsbDeviceBackend();
inline const char *getDefaultControllerID() { return ""; }
inline const char *getSelectedGamepad() { return ""; }
} // namespace Config