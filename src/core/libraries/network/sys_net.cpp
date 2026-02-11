// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#include "common/log.h"
#include "common/singleton.h"
#include "core/filesys/handles.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/network/net.h"
#include "core/libraries/network/neterror.h"
#include "core/libraries/network/sockets.h"
#include <cstring>

namespace Libraries::Net {

static constexpr auto LibNet = "LibNet";

using FDTable = Common::Singleton<Core::FileSys::HandleTable>;

// ----  sys_connect  ----
int PS4_SYSV_ABI sys_connect(OrbisNetId s, const OrbisNetSockaddr *addr,
                             u32 addrlen) {
  auto file = FDTable::Instance().GetSocket(s);
  if (!file) {
    Libraries::Kernel::Error() = ORBIS_NET_EBADF;
    LOG_ERROR(LibNet, "socket id is invalid = {}", static_cast<s32>(s));
    return -1;
  }
  LOG_DEBUG(LibNet, "s = {} ({})", static_cast<s32>(s), file->mguestname);
  int ret = file->socket->Connect(addr, addrlen);
  if (ret < 0) {
    LOG_ERROR(LibNet, "s = {} ({}) returned error code: {}",
              static_cast<s32>(s), file->mguestname,
              static_cast<u32>(Libraries::Kernel::Error()));
    return -1;
  }
  return ret;
}

// ----  sys_bind  ----
int PS4_SYSV_ABI sys_bind(OrbisNetId s, const OrbisNetSockaddr *addr,
                          u32 addrlen) {
  auto file = FDTable::Instance().GetSocket(s);
  if (!file) {
    Libraries::Kernel::Error() = ORBIS_NET_EBADF;
    LOG_ERROR(LibNet, "socket id is invalid = {}", static_cast<s32>(s));
    return -1;
  }
  LOG_DEBUG(LibNet, "s = {} ({})", static_cast<s32>(s), file->mguestname);
  int ret = file->socket->Bind(addr, addrlen);
  if (ret < 0) {
    LOG_ERROR(LibNet, "error code returned: {}",
              static_cast<u32>(Libraries::Kernel::Error()));
    return -1;
  }
  return ret;
}

// ----  sys_accept  ----
int PS4_SYSV_ABI sys_accept(OrbisNetId s, OrbisNetSockaddr *addr,
                            u32 *paddrlen) {
  auto file = FDTable::Instance().GetSocket(s);
  if (!file) {
    Libraries::Kernel::Error() = ORBIS_NET_EBADF;
    LOG_ERROR(LibNet, "socket id is invalid = {}", static_cast<s32>(s));
    return -1;
  }
  LOG_DEBUG(LibNet, "s = {} ({})", static_cast<s32>(s), file->mguestname);
  auto newsock = file->socket->Accept(addr, paddrlen);
  if (!newsock) {
    LOG_ERROR(
        LibNet,
        "s = {} ({}) returned error code creating new socket for accepting: {}",
        static_cast<s32>(s), file->mguestname,
        static_cast<u32>(Libraries::Kernel::Error()));
    return -1;
  }
  // Create new file for accepted socket
  auto fd = FDTable::Instance().CreateHandle();
  auto newfile = FDTable::Instance().GetFile(fd);
  newfile->is_opened = true;
  newfile->type = Core::FileSys::FileType::Socket;
  newfile->socket = newsock;
  return fd;
}

// ----  sys_getpeername  ----
int PS4_SYSV_ABI sys_getpeername(OrbisNetId s, OrbisNetSockaddr *addr,
                                 u32 *namelen) {
  auto file = FDTable::Instance().GetSocket(s);
  if (!file) {
    Libraries::Kernel::Error() = ORBIS_NET_EBADF;
    LOG_ERROR(LibNet, "socket id is invalid = {}", static_cast<s32>(s));
    return -1;
  }
  LOG_DEBUG(LibNet, "s = {} ({})", static_cast<s32>(s), file->mguestname);
  return file->socket->GetPeerName(addr, namelen);
}

} // namespace Libraries::Net