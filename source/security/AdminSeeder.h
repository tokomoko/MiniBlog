#pragma once

#include <MiniSpring.h>
#include "../models/User.h"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

namespace {
std::optional<std::string> readEnv(const char* name) {
  char* buffer = nullptr;
  size_t size = 0;

  if (_dupenv_s(&buffer, &size, name) != 0 || buffer == nullptr)
    return std::nullopt;

  std::string value(buffer);
  free(buffer);
  return value;
}
}  // namespace

class AdminSeeder {
 public:
  static void seed(ServiceProvider& services) {
    auto& manager = services.get<EntityManager>();
    auto& encoder = services.get<IPasswordEncoder>();

    if (!manager.findAll<User>().empty()) return;

    auto username = std::optional<std::string>("admin");
    auto password = std::optional<std::string>("admin123");

    if (!username || !password) {
      std::cerr
          << "[AdminSeeder] users table is empty but MINIBLOG_ADMIN_USERNAME / "
             "MINIBLOG_ADMIN_PASSWORD are not set -- skipping. "
             "Set both and restart to create the first admin.\n";
      return;
    }

    User admin;
    admin.username = *username;
    admin.passwordHash = encoder.encode(*password);
    admin.admin = true;

    manager.persist(admin);
    std::cout << "[AdminSeeder] Created initial admin user '" << *username
              << "'\n";
  }
};