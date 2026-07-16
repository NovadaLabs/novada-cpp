#include "novada/scraper/browser_service.hpp"

#include "novada/detail/envelope.hpp"
#include "novada/detail/form_builder.hpp"
#include "novada/detail/validator.hpp"

namespace novada::scraper {

std::string BrowserService::countries() const {
  nlohmann::json data =
      transport_->do_multipart(transport_->base_url(), "/v1/proxy/browser_area", {});
  // The data shape is undocumented; return its raw JSON text, mirroring Go's
  // json.RawMessage and keeping nlohmann::json off the public API boundary.
  if (data.is_null()) {
    return "";
  }
  return data.dump();
}

BrowserFlowUseList BrowserService::flow_use(const std::string& start,
                                            const std::string& end) const {
  detail::Validator v("Browser.flow_use");
  v.require_str("start_time", start);
  v.require_str("end_time", end);
  v.check();

  detail::FormBuilder form;
  form.req_str("start_time", start);
  form.req_str("end_time", end);

  nlohmann::json data = transport_->do_multipart(transport_->base_url(),
                                                 "/v1/browser_flow/browser_flow_use", form.build());

  BrowserFlowUseList out;
  if (data.contains("list") && data.at("list").is_array()) {
    for (const auto& item : data.at("list")) {
      BrowserFlowUse use;
      from_json(item, use);
      out.list.push_back(use);
    }
  }
  return out;
}

}  // namespace novada::scraper
