#pragma once

#include <MiniSpring.h>
#include "../models/User.h"

class AppUserDetails : public UserDetails {
 public:
  explicit AppUserDetails(User user) : m_user(std::move(user)) {}

  const std::string& username() const override { return m_user.username; }
  const std::string& passwordHash() const override {
    return m_user.passwordHash;
  }
  std::vector<std::string> authorities() const override {
    return m_user.admin ? std::vector<std::string>{"ADMIN"}
                        : std::vector<std::string>{"USER"};
  }

 private:
  User m_user;
};