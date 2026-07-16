#pragma once

#include <string>

#include "novada/detail/transport.hpp"
#include "novada/proxy/types.hpp"

namespace novada::proxy {

// ResidentialService covers residential proxy area listings and traffic (the
// /v1/proxy/* and /v1/residential_flow/* endpoints).
class ResidentialService {
 public:
  explicit ResidentialService(const detail::Transport& transport) : transport_(&transport) {}

  // countries lists residential proxy countries grouped by continent
  // (POST /v1/proxy/domestic_dynamic_area).
  [[nodiscard]] ResidentialAreas countries() const;

  // states lists the states/regions of a country (POST /v1/proxy/city_by_code).
  // code is the country code, e.g. "us", and is required.
  [[nodiscard]] ResidentialStateList states(const std::string& code) const;

  // cities lists the cities of a country region
  // (POST /v1/proxy/region_by_city). Both code and region are required.
  [[nodiscard]] ResidentialCityList cities(const CityParams& params) const;

  // isps lists the ISPs available in a country (POST /v1/proxy/city_isp). code
  // is the country code, e.g. "us", and is required.
  [[nodiscard]] ResidentialISPList isps(const std::string& code) const;

  // balance returns the remaining residential traffic
  // (POST /v1/residential_flow/balance).
  [[nodiscard]] FlowBalance balance() const;

  // consume_log returns the main account's residential traffic consumption
  // over a time range (POST /v1/residential_flow/consume_log). Both bounds are
  // required.
  [[nodiscard]] FlowConsumeLogList consume_log(const TimeRange& range) const;

 private:
  const detail::Transport* transport_;
};

}  // namespace novada::proxy
