#include "novada/scraper/universal_service.hpp"

namespace novada::scraper {

ScraperBalance UniversalService::balance() const {
  nlohmann::json data =
      transport_->do_multipart(transport_->base_url(), "/v1/capture/get_balance", {});
  ScraperBalance out;
  from_json(data, out);
  return out;
}

UsageLogList UniversalService::logs() const {
  nlohmann::json data = transport_->do_multipart(transport_->base_url(), "/v1/capture/logs", {});
  UsageLogList out;
  if (data.contains("list") && data.at("list").is_array()) {
    for (const auto& item : data.at("list")) {
      UsageLog log;
      from_json(item, log);
      out.list.push_back(std::move(log));
    }
  }
  return out;
}

UnitPrices UniversalService::unit() const {
  nlohmann::json data = transport_->do_multipart(transport_->base_url(), "/v1/capture/unit", {});
  UnitPrices out;
  from_json(data, out);
  return out;
}

}  // namespace novada::scraper
