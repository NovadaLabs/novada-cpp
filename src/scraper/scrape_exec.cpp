#include "scraper/scrape_exec.hpp"

#include <nlohmann/json.hpp>

#include "novada/detail/form_builder.hpp"
#include "novada/detail/transport.hpp"
#include "novada/detail/validator.hpp"

namespace novada::scraper::internal {

ScrapeResponse execute_scrape(const novada::detail::Transport& transport,
                              const ScrapeRequest& req) {
  novada::detail::Validator v("Scraper.do_request");
  v.require_str("scraper_name", req.scraper_name);
  v.require_str("scraper_id", req.scraper_id);
  if (req.params.empty()) {
    // require_str cannot express "non-empty list", so record the missing
    // scraper_params field directly by validating a blank sentinel.
    v.require_str("scraper_params", "");
  }
  v.check();

  // Serialize the parameter list to a JSON array string, e.g. [{"url":"..."}].
  nlohmann::json params_json = req.params;
  std::string scraper_params = params_json.dump();

  novada::detail::FormBuilder form;
  form.req_str("scraper_name", req.scraper_name);
  form.req_str("scraper_id", req.scraper_id);
  form.req_str("scraper_params", scraper_params);
  if (req.return_errors) {
    form.req_str("scraper_errors", "true");
  }

  const std::string& host = (req.target == Target::kWebUnblocker) ? transport.web_unblocker_url()
                                                                  : transport.scraper_url();

  ScrapeResponse response;
  response.raw = transport.do_form_urlencoded_raw(host, "/request", form.build());
  return response;
}

}  // namespace novada::scraper::internal
