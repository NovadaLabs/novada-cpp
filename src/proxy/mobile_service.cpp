#include "novada/proxy/mobile_service.hpp"

#include "novada/detail/envelope.hpp"
#include "novada/detail/form_builder.hpp"
#include "novada/detail/validator.hpp"

namespace novada::proxy {

std::string MobileService::countries() const {
  nlohmann::json data =
      transport_->do_multipart(transport_->base_url(), "/v1/proxy/mobile_area", {});
  // Return the raw JSON text of the data payload; its shape is not documented,
  // so callers decode it themselves. This keeps nlohmann::json off the public
  // API boundary while preserving Go's json.RawMessage semantics.
  if (data.is_null()) {
    return "";
  }
  return data.dump();
}

MobileBalance MobileService::balance() const {
  nlohmann::json data =
      transport_->do_multipart(transport_->base_url(), "/v1/mobile_flow/mobile_flow_balance", {});
  MobileBalance out;
  from_json(data, out);
  return out;
}

FlowConsumeLogList MobileService::consume_log(const MobileUseParams& params) const {
  detail::Validator v("Mobile.consume_log");
  v.require_str("start_time", params.start);
  v.require_str("end_time", params.end);
  v.require_str("day_or_hour", params.granularity);
  v.check();

  detail::FormBuilder form;
  form.req_str("start_time", params.start);
  form.req_str("end_time", params.end);
  form.req_str("day_or_hour", params.granularity);

  nlohmann::json data = transport_->do_multipart(transport_->base_url(),
                                                 "/v1/mobile_flow/mobile_flow_use", form.build());

  FlowConsumeLogList out;
  int ignored_total = 0;
  detail::unwrap_list(data, out.list, ignored_total);
  return out;
}

}  // namespace novada::proxy
