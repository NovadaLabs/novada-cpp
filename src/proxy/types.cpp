#include "novada/proxy/types.hpp"

namespace novada::proxy {

// All decoders use j.value("key", default) so missing or newly-added fields
// never throw, preserving forward compatibility with the API.

void from_json(const nlohmann::json& j, FlowBalance& v) {
  v.balance = j.value("balance", 0LL);
  v.expire_time = j.value("expire_time", 0LL);
}

void from_json(const nlohmann::json& j, FlowConsumeLog& v) {
  v.id = j.value("id", 0);
  v.uid = j.value("uid", 0);
  v.balance = j.value("balance", 0LL);
  v.all_buy = j.value("all_buy", 0LL);
  v.use = j.value("use", 0LL);
  v.day = j.value("day", 0LL);
  v.expire_flow = j.value("expire_flow", 0LL);
}

void from_json(const nlohmann::json& j, Account& v) {
  v.created_at = j.value("created_at", 0LL);
  v.updated_at = j.value("updated_at", 0LL);
  v.id = j.value("id", 0);
  v.uid = j.value("uid", 0);
  v.account = j.value("account", std::string());
  v.password = j.value("password", std::string());
  v.status = j.value("status", 0);
  v.residential_balance = j.value("residential_balance", 0LL);
  v.residential_all_buy = j.value("residential_all_buy", 0LL);
  v.residential_status = j.value("residential_status", 0);
  v.dc_balance = j.value("dc_balance", 0LL);
  v.dc_all_buy = j.value("dc_all_buy", 0LL);
  v.dc_status = j.value("dc_status", 0);
  v.isp_balance = j.value("isp_balance", 0LL);
  v.isp_all_buy = j.value("isp_all_buy", 0LL);
  v.isp_status = j.value("isp_status", 0);
  v.flow_type = j.value("flow_type", std::string());
  v.account_type = j.value("account_type", std::string());
  v.check_white_list = j.value("check_white_list", 0);
  v.remark = j.value("remark", std::string());
  v.consumed_residential_flow = j.value("consumed_residential_flow", 0LL);
  v.limit_residential_flow = j.value("limit_residential_flow", 0LL);
  v.consumed_dc_flow = j.value("consumed_dc_flow", 0LL);
  v.limit_dc_flow = j.value("limit_dc_flow", 0LL);
  v.consumed_isp_flow = j.value("consumed_isp_flow", 0LL);
  v.limit_isp_flow = j.value("limit_isp_flow", 0LL);
}

void from_json(const nlohmann::json& j, AccountConsumeLog& v) {
  v.id = j.value("id", 0);
  v.account_id = j.value("account_id", 0);
  v.uid = j.value("uid", 0);
  v.residential_balance = j.value("residential_balance", 0LL);
  v.residential_all_buy = j.value("residential_all_buy", 0LL);
  v.consumed_residential_flow = j.value("consumed_residential_flow", 0LL);
  v.dc_balance = j.value("dc_balance", 0LL);
  v.dc_all_buy = j.value("dc_all_buy", 0LL);
  v.consumed_dc_flow = j.value("consumed_dc_flow", 0LL);
  v.isp_balance = j.value("isp_balance", 0LL);
  v.isp_all_buy = j.value("isp_all_buy", 0LL);
  v.consumed_isp_flow = j.value("consumed_isp_flow", 0LL);
  v.day = j.value("day", 0LL);
}

void from_json(const nlohmann::json& j, WhitelistIP& v) {
  v.created_at = j.value("created_at", 0LL);
  v.updated_at = j.value("updated_at", 0LL);
  v.id = j.value("id", 0);
  v.uid = j.value("uid", 0);
  v.mark_ip = j.value("mark_ip", std::string());
  v.ip = j.value("ip", 0LL);
  v.status = j.value("status", 0);
  v.lock = j.value("lock", false);
  v.lock_uid = j.value("lock_uid", 0);
  v.flag = j.value("flag", 0);
  v.is_biger = j.value("is_biger", 0);
  v.is_limit = j.value("is_limit", 0);
  v.mark = j.value("mark", std::string());
}

// --- Residential -----------------------------------------------------------

void from_json(const nlohmann::json& j, ResidentialCountry& v) {
  v.code = j.value("code", std::string());
  v.continent = j.value("continent", 0);
  v.ip_num = j.value("ip_num", 0);
  v.name = j.value("name", std::string());
  v.name_en = j.value("name_en", std::string());
  v.protocol = j.value("protocol", 0);
}

void from_json(const nlohmann::json& j, ResidentialContinentGroup& v) {
  v.continent = j.value("continent", std::string());
  v.continent_code = j.value("continent_code", 0);
  if (j.contains("list") && j.at("list").is_array()) {
    for (const auto& item : j.at("list")) {
      ResidentialCountry country;
      from_json(item, country);
      v.list.push_back(std::move(country));
    }
  }
}

void from_json(const nlohmann::json& j, ResidentialAreas& v) {
  if (j.contains("continent") && j.at("continent").is_object()) {
    for (const auto& [key, value] : j.at("continent").items()) {
      v.continent[key] = value.is_string() ? value.get<std::string>() : value.dump();
    }
  }
  if (j.contains("country") && j.at("country").is_array()) {
    for (const auto& item : j.at("country")) {
      ResidentialContinentGroup group;
      from_json(item, group);
      v.country.push_back(std::move(group));
    }
  }
}

void from_json(const nlohmann::json& j, ResidentialState& v) {
  v.state = j.value("state", std::string());
}

void from_json(const nlohmann::json& j, ResidentialCity& v) {
  v.code = j.value("code", std::string());
}

void from_json(const nlohmann::json& j, ResidentialISP& v) {
  v.asn = j.value("asn", std::string());
  v.show_name = j.value("show_name", std::string());
}

// --- Mobile ------------------------------------------------------------

void from_json(const nlohmann::json& j, MobileBalance& v) {
  v.balance = j.value("balance", 0LL);
}

// --- Rotating ISP / Rotating DC --------------------------------------------

void from_json(const nlohmann::json& j, ISPCountry& v) {
  v.code = j.value("code", std::string());
  v.node = j.value("node", std::string());
}

void from_json(const nlohmann::json& j, ISPArea& v) {
  if (j.contains("country") && j.at("country").is_array()) {
    for (const auto& item : j.at("country")) {
      ISPCountry country;
      from_json(item, country);
      v.country.push_back(std::move(country));
    }
  }
}

void from_json(const nlohmann::json& j, DCCity& v) {
  v.code = j.value("code", std::string());
}

void from_json(const nlohmann::json& j, DCCountry& v) {
  v.id = j.value("id", 0);
  v.code = j.value("code", std::string());
  v.status = j.value("status", 0);
  v.continents = j.value("continents", 0);
  v.area_code = j.value("area_code", 0);
  v.name_en = j.value("name_en", std::string());
  v.available = j.value("available", 0);
  if (j.contains("city") && j.at("city").is_object()) {
    from_json(j.at("city"), v.city);
  }
}

void from_json(const nlohmann::json& j, DCArea& v) {
  if (j.contains("country") && j.at("country").is_array()) {
    for (const auto& item : j.at("country")) {
      DCCountry country;
      from_json(item, country);
      v.country.push_back(std::move(country));
    }
  }
}

// --- Static (shared by StaticISP / DedicatedDC) -----------------------------

void from_json(const nlohmann::json& j, StaticIP& v) {
  v.id = j.value("id", 0);
  v.ip = j.value("ip", std::string());
  v.port = j.value("port", std::string());
  v.duration = j.value("duration", 0LL);
  v.region = j.value("region", std::string());
  v.expire_time = j.value("expire_time", 0LL);
  v.create_time = j.value("create_time", 0LL);
  v.country = j.value("country", std::string());
  v.country_en = j.value("country_en", std::string());
  v.status = j.value("status", 0);
  v.remark = j.value("remark", std::string());
  v.auto_renew = j.value("auto_renew", 0);
  v.type = j.value("type", 0);
  v.diff_day = j.value("diff_day", 0);
  v.account = j.value("account", std::string());
  v.password = j.value("password", std::string());
}

void from_json(const nlohmann::json& j, RegionNode& v) {
  v.node = j.value("node", std::string());
  v.node_en = j.value("node_en", std::string());
  v.param = j.value("param", 0);
  v.region = j.value("region", std::string());
}

void from_json(const nlohmann::json& j, RegionList& v) {
  if (j.contains("list") && j.at("list").is_object()) {
    for (const auto& [key, value] : j.at("list").items()) {
      std::vector<RegionNode> nodes;
      if (value.is_array()) {
        for (const auto& item : value) {
          RegionNode node;
          from_json(item, node);
          nodes.push_back(std::move(node));
        }
      }
      v.list[key] = std::move(nodes);
    }
  }
}

// --- Unlimited -----------------------------------------------------------

void from_json(const nlohmann::json& j, UnlimitedHost& v) {
  v.id = j.value("id", 0);
  v.create_time = j.value("create_time", 0LL);
  v.expire_time = j.value("expire_time", 0LL);
  v.state = j.value("state", 0);
  v.host = j.value("host", std::string());
  v.user_rate_limit_bytes = j.value("user_rate_limit_bytes", 0LL);
  v.order_id = j.value("order_id", std::string());
  v.days = j.value("days", 0);
  v.duration = j.value("duration", 0);
  v.band_width = j.value("band_width", 0LL);
  v.hardware = j.value("hardware", std::string());
}

// --- ProhibitDomain --------------------------------------------------------

void from_json(const nlohmann::json& j, ProhibitDomain& v) {
  v.created_at = j.value("created_at", 0LL);
  v.updated_at = j.value("updated_at", 0LL);
  v.id = j.value("id", 0);
  v.uid = j.value("uid", 0);
  v.status = j.value("status", 0);
  v.address = j.value("address", std::string());
}

}  // namespace novada::proxy
