// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/nptypes.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpAuth {

// Constants for request limits and offsets
constexpr s32 ORBISNPAUTHREQUESTLIMIT = 0x10;
constexpr s32 ORBISNPAUTHREQUESTIDOFFSET = 0x10000000;

// Structure for creating an asynchronous authentication request
struct OrbisNpAuthCreateAsyncRequestParameter {
    // Size of the parameter
    u64 size;

    // CPU affinity mask
    u64 cpuaffinitymask;

    // Thread priority
    s32 threadpriority;

 // Padding for alignment
 u8 padding[4]; 
};

// Structure for getting an authorization code
struct OrbisNpAuthGetAuthorizationCodeParameter {
    // Size of the parameter
    u64 size;

    // Online ID
    const OrbisNpOnlineId onlineid;

    // Client ID
    const OrbisNpClientId clientid;

    // Scope
    const char scope;
};

// Structure for getting an authorization code (A version)
struct OrbisNpAuthGetAuthorizationCodeParameterA {
    // Size of the parameter
    u64 size;

    // User ID
    s32 userid;

    // Padding for alignment
    u8 padding[4];

    // Client ID
    const OrbisNpClientId clientid;

    // Scope
    const char scope;
};

// Structure for getting an ID token
struct OrbisNpAuthGetIdTokenParameter {
    // Size of the parameter
    u64 size;

    // Online ID
    const OrbisNpOnlineId onlineid;

    // Client ID
    const OrbisNpClientId clientid;

    // Client secret
    const OrbisNpClientSecret clientsecret;

    // Scope
    const char scope;
};

// Structure for getting an ID token (A version)
struct OrbisNpAuthGetIdTokenParameterA {
    // Size of the parameter
    u64 size;

    // User ID
    s32 userid;

    // Padding for alignment
    u8 padding[4];

    // Client ID
    const OrbisNpClientId clientid;

    // Client secret
    const OrbisNpClientSecret client_secret;

    // Scope
    const char scope;
};

// Function to register the library
void RegisterLib(Core::Loader::SymbolsResolver* sym); 
} // namespace Libraries::Np::NpAuth
