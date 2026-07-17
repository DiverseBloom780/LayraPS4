// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// Original implementation for LayraPS4. API signatures derived from
// public PS4 SDK documentation (OpenOrbis).

#pragma once

#include "common/types.h"
#include "core/kernel/module_manager.h"

namespace Core {
namespace Libraries {
namespace UserService {

using OrbisUserServiceUserId = s32;

constexpr s32 ORBIS_USER_SERVICE_USER_ID_SYSTEM = 0xFF;
constexpr s32 ORBIS_USER_SERVICE_USER_ID_INVALID = -1;
constexpr s32 ORBIS_USER_SERVICE_MAX_LOGIN_USERS = 4;
constexpr s32 ORBIS_USER_SERVICE_MAX_USER_NAME_LENGTH = 16;

struct OrbisUserServiceInitializeParams {
  s32 priority;
};

struct OrbisUserServiceLoginUserIdList {
  s32 user_id[ORBIS_USER_SERVICE_MAX_LOGIN_USERS];
};

void RegisterUserService(::Core::Kernel::ModuleManager *module_manager);

} // namespace UserService
} // namespace Libraries
} // namespace Core
