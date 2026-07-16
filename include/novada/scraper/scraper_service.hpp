#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "novada/detail/transport.hpp"

namespace novada::scraper {

class APIService;
class UnblockerService;
class UniversalService;
class BrowserService;

// Target selects which host a scrape /request is sent to.
enum class Target : int {
  // kScraperAPI routes to the Scraper API host (scraper.novada.com).
  kScraperAPI = 0,
  // kWebUnblocker routes to the Web Unblocker host (webunlocker.novada.com).
  kWebUnblocker = 1,
};

// ScrapeRequest is a generic scrape job. It covers every scraper_id; the
// strongly typed helpers under ScraperService::api() build a ScrapeRequest
// for known scrapers.
struct ScrapeRequest {
  // target selects the host (Scraper API or Web Unblocker).
  Target target = Target::kScraperAPI;
  // scraper_name is the site name, e.g. "youtube.com". Required.
  std::string scraper_name;
  // scraper_id identifies the scrape operation, e.g.
  // "youtube_video-post_explore". Required.
  std::string scraper_id;
  // params is the list of per-item parameter maps; it is serialized to a JSON
  // array into the scraper_params form field. Required (at least one item).
  // Values are sent as JSON strings; scrapers needing non-string parameters
  // should encode them accordingly.
  std::vector<std::map<std::string, std::string>> params;
  // return_errors sets scraper_errors=true so the response includes scrape
  // errors.
  bool return_errors = false;
};

// ScrapeResponse holds a raw scrape result. The body format depends on the
// scraper (JSON, CSV or XLSX), so it is returned verbatim.
struct ScrapeResponse {
  // raw is the unparsed response body.
  std::string raw;
};

// ScraperService is the entry point for scraping endpoints. The generic
// do_request() driver submits any scrape job; the sub-service accessors expose
// strongly typed scrapers (api()) and the /v1/* query endpoints (unblocker(),
// universal(), browser()).
class ScraperService {
 public:
  explicit ScraperService(const detail::Transport& transport);
  ~ScraperService();

  ScraperService(ScraperService&&) noexcept;
  ScraperService& operator=(ScraperService&&) noexcept;
  ScraperService(const ScraperService&) = delete;
  ScraperService& operator=(const ScraperService&) = delete;

  // do_request submits a generic scrape job. It serializes params into
  // scraper_params, URL-encodes the form, routes to the host selected by
  // req.target, and returns the raw response body without envelope decoding.
  // Missing scraper_name/scraper_id/params raise a ValidationException before
  // any request is sent.
  [[nodiscard]] ScrapeResponse do_request(const ScrapeRequest& req) const;

  // api exposes strongly typed Scraper API scrapers (youtube, google).
  [[nodiscard]] const APIService& api() const noexcept;
  // unblocker exposes Web Unblocker scraping and its area queries.
  [[nodiscard]] const UnblockerService& unblocker() const noexcept;
  // universal exposes the shared balance/logs/unit-price queries.
  [[nodiscard]] const UniversalService& universal() const noexcept;
  // browser exposes the Browser API area and traffic queries.
  [[nodiscard]] const BrowserService& browser() const noexcept;

 private:
  const detail::Transport* transport_;
  std::unique_ptr<APIService> api_;
  std::unique_ptr<UnblockerService> unblocker_;
  std::unique_ptr<UniversalService> universal_;
  std::unique_ptr<BrowserService> browser_;
};

}  // namespace novada::scraper
