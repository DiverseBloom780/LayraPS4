// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef _WIN32
#define WINSOCK_DEPRECATED_NOWARNINGS
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#if defined(__APPLE__)
#include <net/if_dl.h>
#include <net/route.h>
#endif
#if defined(__linux__)
#include <fstream>
#include <iostream>
#include <sstream>
#endif

#include "common/log.h"
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "common/config.h"
#include "core/networking/networking.h"

namespace NetUtil {

// ----  Stubbed Ethernet address retrieval  ----
static std::array<uint8_t, 6> g_etherAddr = {0x00, 0x11, 0x22,
                                             0x33, 0x44, 0x55};

inline const std::array<uint8_t, 6> &GetEthernetAddr() { return g_etherAddr; }

// ----  Stubbed default-gateway retrieval  ----
static std::string g_defaultGateway = "192.168.1.1";

inline const std::string &GetDefaultGateway() {
  // LAN-play override
  // Note: Config access removed to avoid circular dependency for now, or use
  // forward decl
  return g_defaultGateway;
}

// ----  MAC address string formatter  ----
inline std::string FormatMAC(uint64_t mac) {
  char out[18];
  std::snprintf(out, sizeof(out), "%02x:%02x:%02x:%02x:%02x:%02x",
                (unsigned)(mac >> 40) & 0xFF, (unsigned)(mac >> 32) & 0xFF,
                (unsigned)(mac >> 24) & 0xFF, (unsigned)(mac >> 16) & 0xFF,
                (unsigned)(mac >> 8) & 0xFF, (unsigned)mac & 0xFF);
  return std::string(out);
}

// ----  MAC address parser  ----
inline uint64_t ParseMAC(const std::string &mac) {
  uint64_t val = 0;
  size_t pos = 0;
  for (int i = 5; i >= 0; --i) {
    size_t next = mac.find(':');
    if (next == std::string::npos)
      next = mac.length();
    std::string byte = mac.substr(pos, next - pos);
    try {
      val |= (static_cast<uint64_t>(std::stoul(byte, nullptr, 16)) << (i * 8));
    } catch (...) {
    }
    pos = next + 1;
  }
  return val;
}

} // namespace NetUtil