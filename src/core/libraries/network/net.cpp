// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifdef WIN32
#define WINSOCKDEPRECATEDNOWARNINGS
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include <core/libraries/kernel/kernel.h>
#include <magicenum/magicenum.hpp>
#include "common/assert.h"
#include " common/error.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/filesys/fs.h"
#include "core/ libraries/errorcodes.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/net.h"
#include "netepoll.h"
#include "neterror.h"
#include "netresolver.h"
#include "netutil.h"
#include "netctl.h"
#include " sockets.h"
#include "sysnet.h"

namespace Libraries::Net {

using FDTable = Common::Singleton<Core::FileSys::HandleTable>;

static threadlocal int32t neterrno = 0;

static bool gisNetInitialized = true; // TODO init it properly

static int ConvertFamilies(int family) {
 switch (family) {
 case ORBISNETAFINET:
 return AFINET;
 case ORBIS NETAFINET6:
 return AFINET6;
 default:
 UNREACHABLEMSG("unsupported socket family {}", family);
    }
}

auto NetErrorHandler(auto f) -> decltype(f()) {
    auto result = 0;
    int err;
    int positiveErr;

    do {
        result = f();

        if (result >= 0) {
            return result; // Success
        }

        err = Libraries::Kernel::_Error(); // Standard errno

        // Convert to positive error for comparison
        positiveErr = (err < 0) ? -err : err;

 if ((positiveErr & 0xfff0000) != 0) {
 // Unknown/fatal error range
  sceNetErrnoLoc() = ORBISNETERETURN;
 return - positiveErr; 
 }

        // Retry if interrupted
    } while (positiveErr == ORBISNETEINTR);

    if (positiveErr == ORBISNETENOTSOCK) {
        result = -ORBISNETEBADF;
    } else if (positiveErr == ORBISNETENETINTR) {
        result = -ORBISNETEINTR;
    } else {
        result = -positiveErr;
    }

    sceNetErrnoLoc() = -result;

    return (-result) | ORBISNETERRORBASE; // Convert to official ORBISNETERROR code
}

int PS4SYSVABI in6addrany() {
    LOGERROR(LibNet, "(STUBBED) called");
    return ORBISOK;
}

int PS4SYSVABI in6addrloopback() {
    LOGERROR(LibNet, "(STUBBED) called");
    return ORBISOK;
}

int PS4SYSVABI scenetdummy() {
    LOGERROR(LibNet, "(STUBBED) called");
    return ORBISOK;
}

int PS4SYSVABI scenetin6addrany() {
    LOGERROR(LibNet, "(STUBBED) called");
    return ORBISOK;
}

int PS4SYSVABI scenetin6addrlinklocalallnodes() {
    LOGERROR(LibNet, "(STUBBED) called");
    return ORBISOK;
}

int PS4SYSVABI scenetin6addrlinklocalallrouters() {
    LOGERROR(LibNet, "(STUBBED) called");
    return ORBISOK;
}

int PS4SYSVABI scenetin6addrloopback() {
    LOGERROR(LibNet, "(STUBBED) called");
    return ORBISOK;
}

int PS4SYSVABI scenetin6addrnodelocalallnodes() {
    LOGERROR(LibNet, "(STUBBED) called");
    return ORBISOK;
}

OrbisNetId PS4SYSVABI sceNetAccept(OrbisNetId s, OrbisNetSockaddr addr, u32* paddrlen) {
    if (!gisNetInitialized) {
        return ORBISNETERRORENOTINIT;
    }

    return NetErrorHandler([&] { return sysaccept(s, addr, paddrlen); });
}

int PS4SYSVABI sceNetAddrConfig6GetInfo() {
    LOGERROR(LibNet, "(STUBBED) called");
    return ORBISOK;
}

int PS4SYSVABI sceNetAddrConfig6Start() {
    LOGERROR(LibN
