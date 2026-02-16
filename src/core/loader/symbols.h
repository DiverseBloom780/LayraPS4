// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <string>


namespace Core::Loader {

struct HLEExport {
  std::string name;
  std::string nid;
  uint64_t host_address;
};

} // namespace Core::Loader
