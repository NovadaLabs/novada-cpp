#pragma once

#include <optional>

#include "novada/detail/transport.hpp"
#include "novada/proxy/types.hpp"

namespace novada::proxy {

// UnlimitedService covers unlimited proxy servers (the /v1/unlimited/*
// endpoints).
class UnlimitedService {
 public:
  explicit UnlimitedService(const detail::Transport& transport) : transport_(&transport) {}

  // hosts lists unlimited proxy servers (POST /v1/unlimited/host_list). page
  // and limit default to 1 and 10 when left unset.
  [[nodiscard]] UnlimitedHostList hosts(std::optional<int> page = std::nullopt,
                                        std::optional<int> limit = std::nullopt) const;

 private:
  const detail::Transport* transport_;
};

}  // namespace novada::proxy
