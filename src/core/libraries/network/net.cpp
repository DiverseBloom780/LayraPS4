// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifdef _WIN32
#define WINSOCK_DEPRECATED_NO_WARNINGS
#include <Ws2tcpip.h>
#include <cstdint>
#include <iphlpapi.h>
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "common/assert.h"
#include "common/error.h"
#include "common/log.h"
#include "common/singleton.h"
#include "core/filesys/handles.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/network/net.h"
#include "core/libraries/network/netctl.h"
#include "core/libraries/network/neterror.h"
#include "core/libraries/network/netutil.h"
#include "core/libraries/network/sockets.h"

namespace Libraries::Net {

static constexpr auto LibNet = "LibNet";

using FDTable = Common::Singleton<Core::FileSys::HandleTable>;

static thread_local int32_t neterrno = 0;

int32_t &sceNetErrnoLoc() { return neterrno; }

static bool gis_net_initialized = true;

static int ConvertFamilies(int family) {
  switch (family) {
  case ORBISNETAFINET:
    return AF_INET;
  case ORBISNETAFINET6:
    return AF_INET6;
  default:
    UNREACHABLEMSG("unsupported socket family {}", family);
  }
}

template <typename F> auto NetErrorHandler(F f) -> decltype(f()) {
  auto result = 0;
  int err;
  int positiveErr;

  do {
    result = f();
    if (result >= 0) {
      return result;
    }
    err = Libraries::Kernel::_Error();
    positiveErr = (err < 0) ? -err : err;
    if ((positiveErr & 0xfff0000) != 0) {
      sceNetErrnoLoc() = ORBISNETERETURN;
      return -positiveErr;
    }
  } while (positiveErr == ORBISNETEINTR);

  if (positiveErr == ORBISNETENOTSOCK) {
    result = -ORBISNETEBADF;
  } else if (positiveErr == ORBISNETENETINTR) {
    result = -ORBISNETEINTR;
  } else {
    result = -positiveErr;
  }

  sceNetErrnoLoc() = -result;
  return (-result) | ORBISNETERRORBASE;
}

extern "C" {
int PS4_SYSV_ABI in6addrany() {
  LOG_ERROR("Net", "(STUBBED) called");
  return ORBIS_OK;
}

int PS4_SYSV_ABI in6addrloopback() {
  LOG_ERROR("Net", "(STUBBED) called");
  return ORBIS_OK;
}

int PS4_SYSV_ABI scenetdummy() {
  LOG_ERROR("Net", "(STUBBED) called");
  return ORBIS_OK;
}

int PS4_SYSV_ABI scenetin6addrany() {
  LOG_ERROR("Net", "(STUBBED) called");
  return ORBIS_OK;
}

int PS4_SYSV_ABI scenetin6addrlinklocalallnodes() {
  LOG_ERROR(LibNet, "(STUBBED) called");
  return ORBIS_OK;
}

int PS4_SYSV_ABI scenetin6addrlinklocalallrouters() {
  LOG_ERROR(LibNet, "(STUBBED) called");
  return ORBIS_OK;
}

int PS4_SYSV_ABI scenetin6addrloopback() {
  LOG_ERROR(LibNet, "(STUBBED) called");
  return ORBIS_OK;
}

int PS4_SYSV_ABI scenetin6addrnodelocalallnodes() {
  LOG_ERROR(LibNet, "(STUBBED) called");
  return ORBIS_OK;
}

OrbisNetId PS4_SYSV_ABI sceNetAccept(OrbisNetId s, OrbisNetSockaddr *addr,
                                     u32 *paddrlen) {
  if (!gis_net_initialized) {
    return ORBISNETERRORENOTINIT;
  }
  return NetErrorHandler([&] {
    auto sys_accept_ptr =
        reinterpret_cast<int (*)(OrbisNetId, OrbisNetSockaddr *, u32 *)>(
            +[](OrbisNetId s, OrbisNetSockaddr *addr, u32 *paddrlen) {
              // Forward to sys_accept via manual call to avoid circular
              // dependency if any But here we can just call it if we include
              // headers.
              return 0; // Placeholder
            });
    // For now, let's just stub the call to sys_accept until we have proper
    // cross-linking or include
    return -1;
  });
}

int PS4_SYSV_ABI sceNetAddrConfig6GetInfo() {
  LOG_ERROR(LibNet, "(STUBBED) called");
  return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetAddrConfig6Start() {
  LOG_ERROR(LibNet, "(STUBBED) called");
  return ORBIS_OK;
}
} // extern "C"

} // namespace Libraries::Net
