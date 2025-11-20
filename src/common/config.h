// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <string>
#include "types.h"
#include "core/networking/networking.h"

namespace Config {

    // Forward declaration of UsbBackendType
    enum UsbBackendType : int { Real, SkylandersPortal, InfinityBase, DimensionsToypad };

    // Minimal config entries for now
    std::string GetFoolproofInputConfigFile();
    void load(const std::string& path);
    Core::Networking::Config getNetworkingConfig();
    int getUsbDeviceBackend();

} // namespace Config
