// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <memory>
#include <optional>
#include "common/singleton.h"

namespace Core {
namespace Networking {

enum class Mode {
    Disabled,
    LAN_Play,
    Unofficial_Server,
};

struct Config {
    Mode mode = Mode::Disabled;
    std::string lan_play_relay_address = "switch.lan-play.com:11451";
    std::string guest_ip = "10.13.0.1";
    std::string ezfn_server_address = "";
    std::string gtav_server_address = "";
    bool spoof_psn = false;
};

class NetworkingCore {
public:
    NetworkingCore();
    ~NetworkingCore();

    void Init();
    void Shutdown();
    void UpdateConfig(const Config& new_config);

    // Core HLE functions to intercept guest network calls
    // Placeholder for actual HLE implementation
    std::string ResolveHostname(const std::string& hostname);
    // int sceNetSocket(const std::string& name, int domain, int type, int protocol);
    // int sceNetConnect(int s, const struct sockaddr* name, int namelen);
    // int sceNetSend(int s, const void* buf, size_t len, int flags);
    // int sceNetRecv(int s, void* buf, size_t len, int flags);

    // LAN Play specific functions
    void StartLANPlayTunnel();
    void StopLANPlayTunnel();

private:
    Config current_config;
    bool is_initialized = false;
    // Placeholder for the actual LAN Play tunnel implementation (e.g., a UDP socket and a separate thread)
    std::unique_ptr<void, void(*)(void*)> lan_play_tunnel_handle; 
};

} // namespace Networking
} // namespace Core

// Make NetworkingCore a singleton
namespace Common {
template <typename T>
struct Singleton {
    static T* Instance() {
        static T instance;
        return &instance;
    }
};

template <>
struct Singleton<Core::Networking::NetworkingCore> {
    static Core::Networking::NetworkingCore* Instance() {
        return Singleton<Core::Networking::NetworkingCore>::Instance();
    }
};
} // namespace Common
