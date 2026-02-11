// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#include "common/log.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/network/net.h"
#include "core/libraries/network/neterror.h"
#include "core/libraries/network/sockets.h"
#include <cstring>

namespace Libraries::Net {

static constexpr auto LibNet = "LibNet";

// ----  Stubbed P2P socket class  ----
class P2PSocket {
public:
  int Close() {
    LOG_ERROR(LibNet, "(STUBBED) P2P Close");
    return 0; // success
  }

  int SetSocketOptions(int level, int optname, const void *optval, u32 optlen) {
    LOG_ERROR(LibNet, "(STUBBED) P2P SetSocketOptions");
    return 0; // success
  }

  int GetSocketOptions(int level, int optname, void *optval, u32 optlen) {
    LOG_ERROR(LibNet, "(STUBBED) P2P GetSocketOptions");
    return 0; // success
  }

  int Bind(const OrbisNetSockaddr *addr, u32 addrlen) {
    LOG_ERROR(LibNet, "(STUBBED) P2P Bind");
    return 0; // success
  }

  int Listen(int backlog) {
    LOG_ERROR(LibNet, "(STUBBED) P2P Listen");
    return 0; // success
  }

  int SendMessage(const OrbisNetMsghdr *msg, int flags) {
    LOG_ERROR(LibNet, "(STUBBED) P2P SendMessage");
    Libraries::Kernel::Error() = ORBIS_NET_EAGAIN;
    return -1; // retry
  }

  int SendPacket(const void *msg, u32 len, int flags,
                 const OrbisNetSockaddr *to, u32 tolen) {
    LOG_ERROR(LibNet, "(STUBBED) P2P SendPacket");
    Libraries::Kernel::Error() = ORBIS_NET_EAGAIN;
    return -1; // retry
  }

  int ReceiveMessage(OrbisNetMsghdr *msg, int flags) {
    LOG_ERROR(LibNet, "(STUBBED) P2P ReceiveMessage");
    Libraries::Kernel::Error() = ORBIS_NET_EAGAIN;
    return -1; // retry
  }

  int ReceivePacket(void *buf, u32 len, int flags, OrbisNetSockaddr *from,
                    u32 *fromlen) {
    LOG_ERROR(LibNet, "(STUBBED) P2P ReceivePacket");
    Libraries::Kernel::Error() = ORBIS_NET_EAGAIN;
    return -1; // retry
  }

  int Accept(OrbisNetSockaddr *addr, u32 *addrlen) {
    LOG_ERROR(LibNet, "(STUBBED) P2P Accept");
    Libraries::Kernel::Error() = ORBIS_NET_EAGAIN;
    return -1; // retry
  }

  int Connect(const OrbisNetSockaddr *addr, u32 namelen) {
    LOG_ERROR(LibNet, "(STUBBED) P2P Connect");
    return 0; // success
  }

  int GetSocketAddress(OrbisNetSockaddr *name, u32 namelen) {
    LOG_ERROR(LibNet, "(STUBBED) P2P GetSocketAddress");
    return 0; // success
  }

  int GetPeerName(OrbisNetSockaddr *addr, u32 *namelen) {
    LOG_ERROR(LibNet, "(STUBBED) P2P GetPeerName");
    return 0; // success
  }
};

} // namespace Libraries::Net