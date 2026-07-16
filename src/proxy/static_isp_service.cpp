#include "novada/proxy/static_isp_service.hpp"

#include "novada/detail/envelope.hpp"
#include "novada/detail/form_builder.hpp"
#include "novada/detail/validator.hpp"
#include "proxy/static_helpers.hpp"

namespace novada::proxy {

void StaticISPService::open(const OpenStaticISPParams& params) const {
  detail::Validator v("StaticISP.open");
  v.require_str("ip_type", params.ip_type);
  v.require_str("region", params.region);
  v.require_str("duration", params.duration);
  v.require_nonzero("num", params.num);
  v.check();

  detail::FormBuilder form;
  form.req_str("ip_type", params.ip_type);
  form.req_str("region", params.region);
  form.req_str("duration", params.duration);
  form.req_int("num", params.num);

  transport_->do_multipart(transport_->base_url(), "/v1/static_house/open", form.build());
}

StaticIPList StaticISPService::list(const ListStaticParams& params) const {
  return internal::list_static(*transport_, "/v1/static_house/list", params);
}

std::string StaticISPService::export_list(const ExportStaticParams& params) const {
  return internal::export_static(*transport_, "/v1/static_house/export", params);
}

RegionList StaticISPService::region(const std::string& isp_type) const {
  detail::Validator v("StaticISP.region");
  v.require_str("isp_type", isp_type);
  v.check();

  detail::FormBuilder form;
  form.req_str("isp_type", isp_type);

  nlohmann::json data =
      transport_->do_multipart(transport_->base_url(), "/v1/static_house/region", form.build());
  RegionList out;
  from_json(data, out);
  return out;
}

void StaticISPService::renew(const RenewStaticParams& params) const {
  internal::renew_static(*transport_, "StaticISP.renew", "/v1/static_house/renew", params);
}

void StaticISPService::renew_setting(const RenewSettingParams& params) const {
  internal::renew_setting_static(*transport_, "StaticISP.renew_setting",
                                 "/v1/static_house/renew_setting", "static_house", params);
}

}  // namespace novada::proxy
