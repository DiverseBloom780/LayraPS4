// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include <string>

namespace Common {
struct ElfInfo {
  static std::string GetTitleId() { return "CUSA00000"; }
  static std::string GetTitleName() { return "LayraPS4"; }
};
} // namespace Common
