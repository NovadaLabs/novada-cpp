#include "novada/scraper/types.hpp"

namespace novada::scraper {

// All decoders use j.value("key", default) so missing or newly-added fields
// never throw, preserving forward compatibility with the API.

void from_json(const nlohmann::json& j, GoogleSearchData& v) {
  v.filename = j.value("filename", std::string());
  if (j.contains("html") && !j.at("html").is_null()) {
    v.html = j.at("html").is_string() ? j.at("html").get<std::string>() : j.at("html").dump();
  }
  // "json" is arbitrary scraped output; keep it as raw JSON text.
  if (j.contains("json") && !j.at("json").is_null()) {
    v.json = j.at("json").is_string() ? j.at("json").get<std::string>() : j.at("json").dump();
  }
  v.task_id = j.value("task_id", std::string());
}

void from_json(const nlohmann::json& j, GoogleSearchResult& v) {
  v.code = j.value("code", 0);
  v.cost_time = j.value("cost_time", 0);
  v.msg = j.value("msg", std::string());
  if (j.contains("data") && j.at("data").is_object()) {
    from_json(j.at("data"), v.data);
  }
}

void from_json(const nlohmann::json& j, UnblockerResult& v) {
  v.code = j.value("code", 0);
  v.html = j.value("html", std::string());
  v.msg = j.value("msg", std::string());
  v.msg_detail = j.value("msg_detail", std::string());
  v.use_balance = j.value("use_balance", 0.0);
}

void from_json(const nlohmann::json& j, ScraperBalance& v) {
  // OpenAPI documents the data payload as {"scraper_balance": N}, whereas the
  // Go SDK decoded a bare number. Follow OpenAPI (it is the field truth
  // source), but accept a bare number too for robustness.
  if (j.is_object()) {
    v.scraper_balance = j.value("scraper_balance", 0.0);
  } else if (j.is_number()) {
    v.scraper_balance = j.get<double>();
  }
}

void from_json(const nlohmann::json& j, UsageLog& v) {
  v.time_label = j.value("time_label", std::string());
  v.unlocker_total_cost = j.value("unlocker_total_cost", 0.0);
  v.scraper_total_cost = j.value("scraper_total_cost", 0.0);
  v.unlocker_used_res = j.value("unlocker_used_res", 0LL);
  v.scraper_used_res = j.value("scraper_used_res", 0LL);
  v.scraper_used_flow = j.value("scraper_used_flow", 0LL);
  v.browser_total_cost = j.value("browser_total_cost", 0.0);
  v.browser_used_flow = j.value("browser_used_flow", 0LL);
}

void from_json(const nlohmann::json& j, UnitPrice& v) {
  v.package = j.value("package", std::string());
  v.level = j.value("level", 0);
  v.price = j.value("price", 0.0);
  v.available = j.value("available", 0);
}

namespace {

template <typename T>
std::vector<T> decode_array(const nlohmann::json& j, const char* key) {
  std::vector<T> out;
  if (j.contains(key) && j.at(key).is_array()) {
    out.reserve(j.at(key).size());
    for (const auto& item : j.at(key)) {
      T value{};
      from_json(item, value);
      out.push_back(std::move(value));
    }
  }
  return out;
}

}  // namespace

void from_json(const nlohmann::json& j, UnitPrices& v) {
  v.scraper = decode_array<UnitPrice>(j, "scraper");
  v.unblocker = decode_array<UnitPrice>(j, "unblocker");
}

void from_json(const nlohmann::json& j, Country& v) {
  v.code = j.value("code", std::string());
  v.continent = j.value("continent", 0);
  v.ip_num = j.value("ip_num", 0);
  v.name = j.value("name", std::string());
  v.name_en = j.value("name_en", std::string());
  v.protocol = j.value("protocol", 0);
}

void from_json(const nlohmann::json& j, ContinentGroup& v) {
  v.continent = j.value("continent", std::string());
  v.continent_code = j.value("continent_code", 0);
  v.list = decode_array<Country>(j, "list");
}

void from_json(const nlohmann::json& j, Areas& v) {
  if (j.contains("continent") && j.at("continent").is_object()) {
    for (const auto& [key, value] : j.at("continent").items()) {
      v.continent[key] = value.is_string() ? value.get<std::string>() : value.dump();
    }
  }
  v.country = decode_array<ContinentGroup>(j, "country");
}

void from_json(const nlohmann::json& j, State& v) {
  v.state = j.value("state", std::string());
}

void from_json(const nlohmann::json& j, City& v) {
  v.code = j.value("code", std::string());
}

void from_json(const nlohmann::json& j, ISP& v) {
  v.asn = j.value("asn", std::string());
  v.show_name = j.value("show_name", std::string());
}

void from_json(const nlohmann::json& j, BrowserFlowUse& v) {
  v.id = j.value("id", 0);
  v.uid = j.value("uid", 0);
  v.balance = j.value("balance", 0LL);
  v.all_buy = j.value("all_buy", 0LL);
  v.use = j.value("use", 0LL);
  v.day = j.value("day", 0LL);
  v.expire_flow = j.value("expire_flow", 0LL);
}

}  // namespace novada::scraper
