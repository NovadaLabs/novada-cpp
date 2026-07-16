#include "novada/proxy/dedicated_dc_service.hpp"

#include "novada/detail/envelope.hpp"
#include "novada/detail/form_builder.hpp"
#include "novada/detail/validator.hpp"
#include "proxy/static_helpers.hpp"

namespace novada::proxy {

void DedicatedDCService::open(const OpenDedicatedDCParams& params) const {
  detail::Validator v("DedicatedDC.open");
  v.require_str("region", params.region);
  v.require_str("duration", params.duration);
  v.require_nonzero("num", params.num);
  v.check();

  detail::FormBuilder form;
  form.req_str("region", params.region);
  form.req_str("duration", params.duration);
  form.req_int("num", params.num);

  transport_->do_multipart(transport_->base_url(), "/v1/static/open", form.build());
}

StaticIPList DedicatedDCService::list(const ListStaticParams& params) const {
  return internal::list_static(*transport_, "/v1/static/list", params);
}

std::string DedicatedDCService::export_list(const ExportStaticParams& params) const {
  return internal::export_static(*transport_, "/v1/static/export", params);
}

RegionList DedicatedDCService::region() const {
  nlohmann::json data = transport_->do_multipart(transport_->base_url(), "/v1/static/region", {});
  RegionList out;
  from_json(data, out);
  return out;
}

void DedicatedDCService::renew(const RenewStaticParams& params) const {
  internal::renew_static(*transport_, "DedicatedDC.renew", "/v1/static/renew", params);
}

void DedicatedDCService::renew_setting(const RenewSettingParams& params) const {
  internal::renew_setting_static(*transport_, "DedicatedDC.renew_setting",
                                 "/v1/static/renew_setting", "static", params);
}

}  // namespace novada::proxy
