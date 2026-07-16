#pragma once

#include "novada/detail/transport.hpp"
#include "novada/scraper/types.hpp"

namespace novada::scraper {

// UniversalService exposes the shared scraper account queries (balance, usage
// logs and unit prices) on the general host.
class UniversalService {
 public:
  explicit UniversalService(const detail::Transport& transport) : transport_(&transport) {}

  // balance returns the remaining scraper balance
  // (POST /v1/capture/get_balance).
  [[nodiscard]] ScraperBalance balance() const;

  // logs returns the scraper consumption records (POST /v1/capture/logs).
  [[nodiscard]] UsageLogList logs() const;

  // unit returns the user's capture unit prices (POST /v1/capture/unit).
  [[nodiscard]] UnitPrices unit() const;

 private:
  const detail::Transport* transport_;
};

}  // namespace novada::scraper
