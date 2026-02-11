#pragma once

#include "portal_base.h"

namespace Core::Peripherals {

class SkylandersPortal : public PortalDevice {
public:
  SkylandersPortal() = default;
  ~SkylandersPortal() override = default;

  void ProcessOutputReport(const std::vector<uint8_t> &report) override {
    // Implement Skylanders protocol handling (e.g., color changing, status
    // queries)
  }

  std::vector<uint8_t> GetInputReport() override {
    // Return status/character data
    return std::vector<uint8_t>(32, 0);
  }

  std::string GetName() const override { return "Skylanders Portal of Power"; }
  PortalType GetType() const override { return PortalType::Skylanders; }
};

} // namespace Core::Peripherals
