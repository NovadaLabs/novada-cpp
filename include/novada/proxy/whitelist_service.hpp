#pragma once

#include "novada/detail/transport.hpp"
#include "novada/proxy/types.hpp"

namespace novada::proxy {

// WhitelistService manages IP whitelists via the /v1/white_list/* endpoints.
class WhitelistService {
 public:
  explicit WhitelistService(const detail::Transport& transport) : transport_(&transport) {}

  // add adds an IP to a product's whitelist (POST /v1/white_list/add).
  // product and ip are required. Valid products: 1=Residential, 5=Static ISP,
  // 4=Unlimited.
  void add(const AddWhitelistParams& params) const;

  // list returns whitelisted IPs for a product (POST /v1/white_list/list).
  // product is required; the remaining fields are optional filters.
  [[nodiscard]] WhitelistList list(const ListWhitelistParams& params) const;

  // del removes one or more IPs from a product's whitelist
  // (POST /v1/white_list/del). product and ips are required; pass multiple
  // IPs comma-separated.
  void del(const DeleteWhitelistParams& params) const;

  // remark updates the remark of a whitelist entry
  // (POST /v1/white_list/remark). product and id are required.
  void remark(const RemarkWhitelistParams& params) const;

 private:
  const detail::Transport* transport_;
};

}  // namespace novada::proxy
