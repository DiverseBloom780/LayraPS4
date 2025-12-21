// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/assert.h"
#include "common/types.h"
#include "netctl.h"

namespace Core::Loader {
class SymbolsResolver;
}

// Define our own htonll and ntohll because its not available in some systems/platforms
#ifndef HTONLL
#define HTONLL(x) (((uint64t)htonl((x) & 0xFFFFFFFFUL)) << 32) | htonl((uint32t)((x) >> 32))#endif

#ifndef NTOHLL
#define NTOHLL(x) (((uint64t)ntohl((x) & 0xFFFFFFFFUL)) << 32) | ntohl ((uint32t) ((x) >> 32))#endif

namespace Libraries::Net {

static int ConvertFamilies(int family);

enum OrbisNetFamily : u32 {
 ORBISNETAFUNIX = 1,
 ORBISNETAFINET = 2,
 ORBISNETAFINET6 = 28,
}; 
enum OrbisNetSocketType : u32 {
 ORBISNETSOCKSTREAM = 1,
 ORBISNETSOCKDGRAM = 2,
 ORBISNET SOCKRAW = 3,
 ORBISNETSOCKDGRAMP2P = 6,
 ORBISNETSOCKSTREAMP2P = 10
};

enum OrbisNetProtocol : u32 {
    ORBISNETIPPROTOIP = 0,
    ORBISNETIPPROTOICMP = 1,
    ORBISNETIPPROTOIGMP = 2,
    ORBISNETIPPROTOTCP = 6,
    ORBISNETIPPROTOUDP = 17,
    ORBISNETIPPROTOIPV6 = 41,
    ORBISNETSOLSOCKET = 0xFFFF
};

constexpr std::stringview NameOf(OrbisNetProtocol p) {
 switch (p) {
 case ORBISNETIPPROTOIP:
 return "ORBISNET IPPROTOIP"; 
 case ORBISNETIPPROTOICMP:
 return "ORBISNETIPPROTOICMP"; 
 case ORBISNETIPPROTOIGMP:
 return "ORBISNETIPPROTOIGMP"; 
 case ORBISNETIPPROTOTCP:
 return "ORBISNETIPPROTOTCP"; 
 case ORBISNETIPPROTOUDP:
 return "ORBISNETIPPROTOUDP"; 
 case ORBISNETIPPROTOIPV6:
 return "ORBISNETIPPROTOIPV6"; 
 case ORBISNETSOLSOCKET:
 return "ORBISNETSOLSOCKET"; 
 default:
 UNREACHABLEMSG("{}", (u32)p);
 } 
}

enum OrbisNetSocketIpOption : u32 {
    ORBISNETIPHDRINCL = 2,
    ORBISNETIPTOS = 3,
    ORBISNETIPTTL = 4,
    ORBISNETIPMULTICASTIF = 9,
    ORBISNETIPMULTICASTTTL = 10,
    ORBISNETIPMULTICASTLOOP = 11,
    ORBISNETIPADDMEMBERSHIP = 12,
    ORBISNETIPDROPMEMBERSHIP = 13,
    ORBISNETIPTTLCHK = 23,
    ORBISNETIPMAXTTL = 24,
};

enum OrbisNetSocketTcpOption : u32 {
    ORBISNETTCPNODELAY = 1,
    ORBISNETTCPMAXSEG = 2,
    ORBISNETTCPMSSTOADVERTISE = 3,
};

enum OrbisNetSocketSoOption : u32 {
    ORBISNETSOREUSEADDR = 0x00000004,
    ORBISNETSOKEEPALIVE = 0x00000008,
    ORBISNETSOBROADCAST = 0x00000020,
    ORBISNETSOLINGER = 0x00000080,
    ORBISNETSOREUSEPORT = 0x00000200,
    ORBISNETSOONESBCAST = 0x00010000,
    ORBISNETSOUSECRYPTO = 0x00020000,
    ORBISNETSOUSESIGNATURE = 0x00040000,
    ORBISNETSOSNDBUF = 0x1001,
    ORBISNETSORCVBUF = 0x1002,
    ORBISNETSOERROR = 0x1007,
    ORBISNETSOTYPE = 0x1008,
    ORBISNETSOSNDTIMEO = 0x1105,
    ORBIS
