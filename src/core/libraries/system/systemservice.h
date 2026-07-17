// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/kernel/module_manager.h"

namespace Core {
namespace Libraries {
namespace SystemService {

void RegisterSystemService(::Core::Kernel::ModuleManager *module_manager);

} // namespace SystemService
} // namespace Libraries
} // namespace Core
