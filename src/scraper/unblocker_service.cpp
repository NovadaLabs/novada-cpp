#include "novada/scraper/unblocker_service.hpp"

#include "novada/detail/envelope.hpp"
#include "novada/detail/form_builder.hpp"
#include "novada/detail/validator.hpp"

namespace novada::scraper {

UnblockerResult UnblockerService::scrape(const UnblockerParams& params) const {
  detail::Validator v("Unblocker.scrape");
  v.require_str("target_url", params.target_url);
  v.check();

  // response_format defaults to "html" when the caller leaves it empty.
  std::string format = params.response_format.empty() ? "html" : params.response_format;

  detail::FormBuilder form;
  form.req_str("target_url", params.target_url);
  form.req_str("response_format", format);
  form.opt_bool("js_render", params.js_render);
  form.opt_str("headers", params.headers);
  form.opt_str("cookies", params.cookies);
  form.opt_str("country", params.country);
  if (params.wait_ms > 0) {
    form.req_int("wait_ms", params.wait_ms);
  }
  form.opt_str("wait_selector", params.wait_selector);
  form.opt_bool("follow_redirects", params.follow_redirects);
  form.opt_bool("block_resources", params.block_resources);
  form.opt_bool("clear", params.clear);
  form.opt_int("auto_runs", params.auto_runs);

  nlohmann::json data =
      transport_->do_form_urlencoded(transport_->web_unblocker_url(), "/request", form.build());

  UnblockerResult out;
  from_json(data, out);
  return out;
}

Areas UnblockerService::countries() const {
  nlohmann::json data =
      transport_->do_multipart(transport_->base_url(), "/v1/proxy/unblocker_area", {});
  Areas out;
  from_json(data, out);
  return out;
}

StateList UnblockerService::states(const std::string& code) const {
  detail::Validator v("Unblocker.states");
  v.require_str("code", code);
  v.check();

  detail::FormBuilder form;
  form.req_str("code", code);

  nlohmann::json data = transport_->do_multipart(
      transport_->base_url(), "/v1/proxy/unblocker_area_by_country", form.build());

  StateList out;
  if (data.contains("data") && data.at("data").is_array()) {
    for (const auto& item : data.at("data")) {
      State state;
      from_json(item, state);
      out.data.push_back(std::move(state));
    }
  }
  return out;
}

CityList UnblockerService::cities(const std::string& code, const std::string& region) const {
  detail::Validator v("Unblocker.cities");
  v.require_str("code", code);
  v.require_str("region", region);
  v.check();

  detail::FormBuilder form;
  form.req_str("code", code);
  form.req_str("region", region);

  nlohmann::json data = transport_->do_multipart(transport_->base_url(),
                                                 "/v1/proxy/unblocker_city_by_area", form.build());

  CityList out;
  if (data.contains("data") && data.at("data").is_array()) {
    for (const auto& item : data.at("data")) {
      City city;
      from_json(item, city);
      out.data.push_back(std::move(city));
    }
  }
  return out;
}

ISPList UnblockerService::isps(const std::string& code) const {
  detail::Validator v("Unblocker.isps");
  v.require_str("code", code);
  v.check();

  detail::FormBuilder form;
  form.req_str("code", code);

  nlohmann::json data = transport_->do_multipart(transport_->base_url(),
                                                 "/v1/proxy/unblocker_city_isp", form.build());

  ISPList out;
  int ignored_total = 0;
  detail::unwrap_list(data, out.list, ignored_total);
  return out;
}

}  // namespace novada::scraper
