#pragma once

#include "novada/detail/transport.hpp"
#include "novada/proxy/account_service.hpp"
#include "novada/proxy/dedicated_dc_service.hpp"
#include "novada/proxy/mobile_service.hpp"
#include "novada/proxy/prohibit_domain_service.hpp"
#include "novada/proxy/residential_service.hpp"
#include "novada/proxy/rotating_service.hpp"
#include "novada/proxy/static_isp_service.hpp"
#include "novada/proxy/unlimited_service.hpp"
#include "novada/proxy/whitelist_service.hpp"

namespace novada::proxy {

// ProxyService is the entry point for proxy management endpoints. Reach the
// sub-services through its accessor methods, e.g.
// client.proxy().whitelist().list(...).
class ProxyService {
 public:
  explicit ProxyService(const detail::Transport& transport)
      : whitelist_(transport),
        account_(transport),
        residential_(transport),
        mobile_(transport),
        rotating_isp_(transport),
        rotating_dc_(transport),
        static_isp_(transport),
        dedicated_dc_(transport),
        unlimited_(transport),
        prohibit_domain_(transport) {}

  // whitelist manages IP whitelists (/v1/white_list/*).
  [[nodiscard]] const WhitelistService& whitelist() const { return whitelist_; }
  // account manages proxy sub-accounts (/v1/proxy_account/*).
  [[nodiscard]] const AccountService& account() const { return account_; }
  // residential covers residential proxy areas and traffic.
  [[nodiscard]] const ResidentialService& residential() const { return residential_; }
  // mobile covers mobile proxy areas and traffic.
  [[nodiscard]] const MobileService& mobile() const { return mobile_; }
  // rotating_isp covers rotating ISP proxy areas and traffic.
  [[nodiscard]] const RotatingISPService& rotating_isp() const { return rotating_isp_; }
  // rotating_dc covers rotating datacenter proxy areas and traffic.
  [[nodiscard]] const RotatingDCService& rotating_dc() const { return rotating_dc_; }
  // static_isp covers static ISP proxies (/v1/static_house/*).
  [[nodiscard]] const StaticISPService& static_isp() const { return static_isp_; }
  // dedicated_dc covers dedicated datacenter proxies (/v1/static/*).
  [[nodiscard]] const DedicatedDCService& dedicated_dc() const { return dedicated_dc_; }
  // unlimited covers unlimited proxy servers (/v1/unlimited/*).
  [[nodiscard]] const UnlimitedService& unlimited() const { return unlimited_; }
  // prohibit_domain manages the blocked-domain list (/v1/prohibit_domain/*).
  [[nodiscard]] const ProhibitDomainService& prohibit_domain() const { return prohibit_domain_; }

 private:
  WhitelistService whitelist_;
  AccountService account_;
  ResidentialService residential_;
  MobileService mobile_;
  RotatingISPService rotating_isp_;
  RotatingDCService rotating_dc_;
  StaticISPService static_isp_;
  DedicatedDCService dedicated_dc_;
  UnlimitedService unlimited_;
  ProhibitDomainService prohibit_domain_;
};

}  // namespace novada::proxy
