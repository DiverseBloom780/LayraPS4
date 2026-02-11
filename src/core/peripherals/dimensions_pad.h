#pragma once

#include "portal_base.h"

namespace Core::Peripherals {

class DimensionsPad : public PortalDevice {
public:
  DimensionsPad() = default;
  ~DimensionsPad() override = default;

  void ProcessOutputReport(const std::vector<uint8_t> &report) override {
    // Implement Lego Dimensions Toypad protocol (LED colors, etc.)
  }

  std::vector<uint8_t> GetInputReport() override {
    return std::vector<uint8_t>(32, 0);
  }

  std::string GetName() const override { return "Lego Dimensions Toypad"; }
  PortalType GetType() const override { return PortalType::LegoDimensions; }
};

} // namespace Core::Peripherals
