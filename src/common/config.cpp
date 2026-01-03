// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#include "config.h"
#include "log.h"

namespace Config {
    std::string GetFoolproofInputConfigFile() { return "config.toml"; }
    void load(const std::string& path) { Log::log("Loading config from: " + path); }
    Core::Networking::Config getNetworkingConfig() {
        Core::Networking::Config config;
        config.lan_play_enabled = true;
        config.dns_spoofing_enabled = true;
        return config;
    }
    int getUsbDeviceBackend() { return static_cast<int>(UsbBackendType::Real); }
} // namespace Config