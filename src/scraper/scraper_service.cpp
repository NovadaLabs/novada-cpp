#include "novada/scraper/scraper_service.hpp"

#include "novada/scraper/api_service.hpp"
#include "novada/scraper/browser_service.hpp"
#include "novada/scraper/unblocker_service.hpp"
#include "novada/scraper/universal_service.hpp"
#include "scraper/scrape_exec.hpp"

namespace novada::scraper {

ScraperService::ScraperService(const detail::Transport& transport)
    : transport_(&transport),
      api_(std::make_unique<APIService>(transport)),
      unblocker_(std::make_unique<UnblockerService>(transport)),
      universal_(std::make_unique<UniversalService>(transport)),
      browser_(std::make_unique<BrowserService>(transport)) {}

ScraperService::~ScraperService() = default;
ScraperService::ScraperService(ScraperService&&) noexcept = default;
ScraperService& ScraperService::operator=(ScraperService&&) noexcept = default;

ScrapeResponse ScraperService::do_request(const ScrapeRequest& req) const {
  return internal::execute_scrape(*transport_, req);
}

const APIService& ScraperService::api() const noexcept {
  return *api_;
}
const UnblockerService& ScraperService::unblocker() const noexcept {
  return *unblocker_;
}
const UniversalService& ScraperService::universal() const noexcept {
  return *universal_;
}
const BrowserService& ScraperService::browser() const noexcept {
  return *browser_;
}

}  // namespace novada::scraper
