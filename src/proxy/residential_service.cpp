#include "novada/proxy/residential_service.hpp"

#include "novada/detail/envelope.hpp"
#include "novada/detail/form_builder.hpp"
#include "novada/detail/validator.hpp"
#include "proxy/flow_helpers.hpp"

namespace novada::proxy {

ResidentialAreas ResidentialService::countries() const {
  nlohmann::json data =
      transport_->do_multipart(transport_->base_url(), "/v1/proxy/domestic_dynamic_area", {});
  ResidentialAreas out;
  from_json(data, out);
  return out;
}

ResidentialStateList ResidentialService::states(const std::string& code) const {
  detail::Validator v("Residential.states");
  v.require_str("code", code);
  v.check();

  detail::FormBuilder form;
  form.req_str("code", code);

  nlohmann::json data =
      transport_->do_multipart(transport_->base_url(), "/v1/proxy/city_by_code", form.build());

  ResidentialStateList out;
  if (data.contains("data") && data.at("data").is_array()) {
    for (const auto& item : data.at("data")) {
      ResidentialState state;
      from_json(item, state);
      out.data.push_back(std::move(state));
    }
  }
  return out;
}

ResidentialCityList ResidentialService::cities(const CityParams& params) const {
  detail::Validator v("Residential.cities");
  v.require_str("code", params.code);
  v.require_str("region", params.region);
  v.check();

  detail::FormBuilder form;
  form.req_str("code", params.code);
  form.req_str("region", params.region);

  nlohmann::json data =
      transport_->do_multipart(transport_->base_url(), "/v1/proxy/region_by_city", form.build());

  ResidentialCityList out;
  if (data.contains("data") && data.at("data").is_array()) {
    for (const auto& item : data.at("data")) {
      ResidentialCity city;
      from_json(item, city);
      out.data.push_back(std::move(city));
    }
  }
  return out;
}

ResidentialISPList ResidentialService::isps(const std::string& code) const {
  detail::Validator v("Residential.isps");
  v.require_str("code", code);
  v.check();

  detail::FormBuilder form;
  form.req_str("code", code);

  nlohmann::json data =
      transport_->do_multipart(transport_->base_url(), "/v1/proxy/city_isp", form.build());

  ResidentialISPList out;
  int ignored_total = 0;
  detail::unwrap_list(data, out.list, ignored_total);
  return out;
}

FlowBalance ResidentialService::balance() const {
  nlohmann::json data =
      transport_->do_multipart(transport_->base_url(), "/v1/residential_flow/balance", {});
  FlowBalance out;
  from_json(data, out);
  return out;
}

FlowConsumeLogList ResidentialService::consume_log(const TimeRange& range) const {
  return internal::consume_log_over_range(*transport_, "Residential.consume_log",
                                          "/v1/residential_flow/consume_log", range);
}

}  // namespace novada::proxy
