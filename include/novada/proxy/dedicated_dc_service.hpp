#pragma once

#include <string>

#include "novada/detail/transport.hpp"
#include "novada/proxy/types.hpp"

namespace novada::proxy {

// DedicatedDCService covers dedicated datacenter proxies (the /v1/static/*
// endpoints). It shares the StaticIP/RegionList response types with
// StaticISPService.
class DedicatedDCService {
 public:
  explicit DedicatedDCService(const detail::Transport& transport) : transport_(&transport) {}

  // open purchases dedicated datacenter IPs (POST /v1/static/open). All fields
  // of params are required.
  void open(const OpenDedicatedDCParams& params) const;

  // list returns purchased dedicated datacenter IPs (POST /v1/static/list).
  // page and limit default to 1 and 10.
  [[nodiscard]] StaticIPList list(const ListStaticParams& params) const;

  // export_list returns the dedicated datacenter IP list as a file stream
  // (POST /v1/static/export). The bytes are typically CSV.
  [[nodiscard]] std::string export_list(const ExportStaticParams& params) const;

  // region lists the available dedicated datacenter regions
  // (POST /v1/static/region).
  [[nodiscard]] RegionList region() const;

  // renew renews dedicated datacenter IPs (POST /v1/static/renew).
  void renew(const RenewStaticParams& params) const;

  // renew_setting updates dedicated datacenter auto-renewal settings
  // (POST /v1/static/renew_setting).
  void renew_setting(const RenewSettingParams& params) const;

 private:
  const detail::Transport* transport_;
};

}  // namespace novada::proxy
