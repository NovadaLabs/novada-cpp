#pragma once

#include "novada/detail/transport.hpp"
#include "novada/proxy/types.hpp"

namespace novada::proxy {

// AccountService manages proxy sub-accounts via the /v1/proxy_account/*
// endpoints.
class AccountService {
 public:
  explicit AccountService(const detail::Transport& transport) : transport_(&transport) {}

  // create adds a proxy sub-account (POST /v1/proxy_account/create).
  // product, account, password and status are required.
  void create(const CreateAccountParams& params) const;

  // list returns proxy sub-accounts (POST /v1/proxy_account/list). product is
  // required; page and limit default to 1 and 10.
  [[nodiscard]] AccountList list(const ListAccountParams& params) const;

  // update modifies a proxy sub-account (POST /v1/proxy_account/update). id,
  // account and password are required.
  void update(const UpdateAccountParams& params) const;

  // consume_log returns per-day traffic consumption for an account
  // (POST /v1/proxy_account/consume_log). account_id is required; page and
  // limit default to 1 and 10.
  [[nodiscard]] AccountConsumeLogList consume_log(const ConsumeLogParams& params) const;

 private:
  const detail::Transport* transport_;
};

}  // namespace novada::proxy
