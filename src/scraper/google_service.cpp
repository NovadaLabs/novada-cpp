#include "novada/scraper/google_service.hpp"

#include "novada/detail/envelope.hpp"
#include "novada/detail/form_builder.hpp"
#include "novada/detail/validator.hpp"

namespace novada::scraper {

GoogleSearchResult GoogleService::search(const GoogleSearchParams& params) const {
  detail::Validator v("Google.search");
  v.require_str("q", params.query);
  v.check();

  detail::FormBuilder form;
  form.req_str("scraper_name", "google.com");
  form.req_str("scraper_id", "google_search");
  form.req_str("q", params.query);
  // json=1 requests JSON output (default); json=0 requests HTML output.
  form.req_str("json", params.html ? "0" : "1");
  form.opt_str("device", params.device);
  form.opt_bool("render_js", params.render_js);
  form.opt_bool("no_cache", params.no_cache);
  form.opt_bool("ai_overview", params.ai_overview);
  form.opt_str("domain", params.domain);
  form.opt_str("country", params.country);
  form.opt_str("hl", params.hl);
  if (params.return_errors) {
    form.req_str("scraper_errors", "true");
  }

  nlohmann::json data =
      transport_->do_form_urlencoded(transport_->scraper_url(), "/request", form.build());

  GoogleSearchResult out;
  from_json(data, out);
  return out;
}

}  // namespace novada::scraper
