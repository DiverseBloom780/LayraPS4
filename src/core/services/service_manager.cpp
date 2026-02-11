#include "service_manager.h"
#include <iostream>

namespace Core {
namespace Services {

ServiceManager::ServiceManager() {
  std::cout << "[Services] Service Manager initialized.\n";
}

ServiceManager::~ServiceManager() {}

void ServiceManager::RegisterService(std::unique_ptr<Service> service) {
  std::cout << "[Services] Registering service: " << service->GetName() << "\n";
  services[service->GetName()] = std::move(service);
}

Service *ServiceManager::GetService(const std::string &name) {
  auto it = services.find(name);
  if (it != services.end()) {
    return it->second.get();
  }
  return nullptr;
}

void ServiceManager::InitializeAll() {
  for (auto &[name, service] : services) {
    std::cout << "[Services] Initializing service: " << name << "\n";
    service->Initialize();
  }
}

} // namespace Services
} // namespace Core
