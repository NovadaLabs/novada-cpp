#pragma once

#include "novada/detail/transport.hpp"
#include "novada/scraper/types.hpp"

namespace novada::scraper {

// GoogleService groups Google scrapers (scraper_name "google.com"). Unlike the
// generic scrape pipeline, Google scrapers send their parameters as flat form
// fields (not a scraper_params JSON array) and their response goes through the
// standard envelope, so this service builds the form directly and decodes a
// structured result.
class GoogleService {
 public:
  explicit GoogleService(const detail::Transport& transport) : transport_(&transport) {}

  // search scrapes Google search results (scraper_id "google_search") and
  // decodes the structured result. params.query is required.
  [[nodiscard]] GoogleSearchResult search(const GoogleSearchParams& params) const;

 private:
  const detail::Transport* transport_;
};

}  // namespace novada::scraper
