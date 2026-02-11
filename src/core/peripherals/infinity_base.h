#pragma once

#include "portal_base.h"

namespace Core::Peripherals {

class InfinityBase : public PortalDevice {
public:
  InfinityBase() = default;
  ~InfinityBase() override = default;

  void ProcessOutputReport(const std::vector<uint8_t> &report) override {
    // Implement Disney Infinity protocol
  }

  std::vector<uint8_t> GetInputReport() override {
    return std::vector<uint8_t>(32, 0);
  }

  std::string GetName() const override { return "Disney Infinity Base"; }
  PortalType GetType() const override { return PortalType::DisneyInfinity; }
};

} // namespace Core::Peripherals
