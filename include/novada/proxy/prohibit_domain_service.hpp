#pragma once

#include <string>

#include "novada/detail/transport.hpp"
#include "novada/proxy/types.hpp"

namespace novada::proxy {

// ProhibitDomainService manages the blocked-domain list (the
// /v1/prohibit_domain/* endpoints).
class ProhibitDomainService {
 public:
  explicit ProhibitDomainService(const detail::Transport& transport) : transport_(&transport) {}

  // add adds a blocked domain (POST /v1/prohibit_domain/add). address is
  // required.
  void add(const std::string& address) const;

  // list returns the blocked-domain list (POST /v1/prohibit_domain/list).
  [[nodiscard]] ProhibitDomainList list() const;

  // del removes blocked domains (POST /v1/prohibit_domain/del). Set params.all
  // to delete every entry, otherwise params.id is required.
  void del(const DeleteProhibitParams& params) const;

 private:
  const detail::Transport* transport_;
};

}  // namespace novada::proxy
