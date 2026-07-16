#pragma once

#include <string>

#include "novada/detail/transport.hpp"
#include "novada/scraper/types.hpp"

namespace novada::scraper {

// UnblockerService exposes Web Unblocker scraping plus its area queries. The
// area queries are /v1/* APIs on the general host; the scrape itself uses
// scrape() (which routes to the Web Unblocker host).
class UnblockerService {
 public:
  explicit UnblockerService(const detail::Transport& transport) : transport_(&transport) {}

  // scrape submits a Web Unblocker scrape job (POST /request on the Web
  // Unblocker host) and decodes the structured result. params.target_url is
  // required; response_format defaults to "html" when empty.
  [[nodiscard]] UnblockerResult scrape(const UnblockerParams& params) const;

  // countries lists Web Unblocker countries (POST /v1/proxy/unblocker_area).
  [[nodiscard]] Areas countries() const;

  // states lists the states of a country
  // (POST /v1/proxy/unblocker_area_by_country). code is required.
  [[nodiscard]] StateList states(const std::string& code) const;

  // cities lists the cities of a region
  // (POST /v1/proxy/unblocker_city_by_area). Both code and region are
  // required.
  [[nodiscard]] CityList cities(const std::string& code, const std::string& region) const;

  // isps lists the carriers of a country (POST /v1/proxy/unblocker_city_isp).
  // code is required.
  [[nodiscard]] ISPList isps(const std::string& code) const;

 private:
  const detail::Transport* transport_;
};

}  // namespace novada::scraper
