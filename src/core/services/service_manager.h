// SPDX-FileCopyrightText: Copyright 2025 LayraPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>


namespace Core {

namespace Services {

class Service {
public:
  virtual ~Service() = default;
  virtual std::string GetName() const = 0;
  virtual void Initialize() = 0;
};

class ServiceManager {
public:
  ServiceManager();
  ~ServiceManager();

  void RegisterService(std::unique_ptr<Service> service);
  Service *GetService(const std::string &name);

  void InitializeAll();

private:
  std::map<std::string, std::unique_ptr<Service>> services;
};

} // namespace Services
} // namespace Core
