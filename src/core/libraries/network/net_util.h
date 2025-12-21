// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include "common/types.h"

namespace Libraries::Net {
struct OrbisNetInAddr;
}

namespace NetUtil {

class NetUtilInternal {
public:
    // Default constructor
    explicit NetUtilInternal() = default;

    // Default destructor
    ~NetUtilInternal() = default;

private:
    // Ethernet address
    std::array<u8, 6> etheraddress{};

    // Default gateway
    std::string defaultgateway{};

    // Netmask
    std::string netmask{};

    // IP address
    std::string ip{};

    // Mutex for thread safety
    std::mutex m_mutex;

public:
    // Get the Ethernet address
    const std::array<u8, 6>& GetEthernetAddr() const;

    // Get the default gateway
    const std::string& GetDefaultGateway() const;

    // Get the netmask
    const std::string& GetNetmask() const;

    // Get the IP address
    const std::string& GetIp() const;

    // Retrieve the Ethernet address
    bool RetrieveEthernetAddr();

    // Retrieve the default gateway
    bool RetrieveDefaultGateway();

    // Retrieve the netmask
    bool RetrieveNetmask();

    // Retrieve the IP address
    bool RetrieveIp();

    // Resolve a hostname to an IP address
    int ResolveHostname(const char hostname, Libraries::Net::OrbisNetInAddr addr);
};

} // namespace NetUtil
