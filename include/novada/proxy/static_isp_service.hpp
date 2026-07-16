#pragma once

#include <string>

#include "novada/detail/transport.hpp"
#include "novada/proxy/types.hpp"

namespace novada::proxy {

// StaticISPService covers static ISP proxies (the /v1/static_house/*
// endpoints).
class StaticISPService {
 public:
  explicit StaticISPService(const detail::Transport& transport) : transport_(&transport) {}

  // open purchases static ISP IPs (POST /v1/static_house/open). All fields of
  // params are required.
  void open(const OpenStaticISPParams& params) const;

  // list returns purchased static ISP IPs (POST /v1/static_house/list). page
  // and limit default to 1 and 10.
  [[nodiscard]] StaticIPList list(const ListStaticParams& params) const;

  // export_list returns the static ISP IP list as a file stream
  // (POST /v1/static_house/export). The bytes are typically CSV.
  [[nodiscard]] std::string export_list(const ExportStaticParams& params) const;

  // region lists the available static ISP regions
  // (POST /v1/static_house/region). isp_type is required: "isp-resi" = static
  // residential, "isp-resi-hq" = premium.
  [[nodiscard]] RegionList region(const std::string& isp_type) const;

  // renew renews static ISP IPs (POST /v1/static_house/renew).
  void renew(const RenewStaticParams& params) const;

  // renew_setting updates static ISP auto-renewal settings
  // (POST /v1/static_house/renew_setting).
  void renew_setting(const RenewSettingParams& params) const;

 private:
  const detail::Transport* transport_;
};

}  // namespace novada::proxy
