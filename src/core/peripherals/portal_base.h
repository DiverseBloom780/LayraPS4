// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include <cstdint>
#include <string>
#include <vector>


namespace Core::Peripherals {

enum class PortalType { None, Skylanders, DisneyInfinity, LegoDimensions };

class PortalDevice {
public:
  virtual ~PortalDevice() = default;

  // Process an incoming HID report from the host (emulator)
  virtual void ProcessOutputReport(const std::vector<uint8_t> &report) = 0;

  // Generate an input report for the host (emulator)
  virtual std::vector<uint8_t> GetInputReport() = 0;

  // Get the device name
  virtual std::string GetName() const = 0;

  // Get the portal type
  virtual PortalType GetType() const = 0;
};

} // namespace Core::Peripherals
