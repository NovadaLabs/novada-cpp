#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace novada::proxy {

// Product identifies a Novada proxy product line. The numeric values are
// fixed by the API; not every product is valid for every endpoint (see each
// method's documentation).
enum class Product : int {
  kResidential = 1,
  kRotatingISP = 2,
  kRotatingDC = 3,
  kUnlimited = 4,
  kStaticISP = 5,
  kUnblocker = 7,
  kMobile = 9,
  kBrowserAPI = 10,
};

// --- Shared ------------------------------------------------------------

// TimeRange is a start/end window used by the traffic-consumption endpoints.
// Both bounds use the "2006-01-02 15:04:05" layout and are required.
struct TimeRange {
  std::string start;  // required; "2006-01-02 15:04:05"
  std::string end;    // required; "2006-01-02 15:04:05"
};

// FlowBalance is the remaining-traffic payload shared by the residential,
// rotating ISP and rotating datacenter balance endpoints. expire_time is a
// Unix timestamp (seconds) and is zero for products that do not report it.
struct FlowBalance {
  long long balance = 0;
  long long expire_time = 0;
};
void from_json(const nlohmann::json& j, FlowBalance& v);

// FlowConsumeLog is a single traffic-consumption record shared by the
// residential, rotating ISP, rotating datacenter and mobile consumption
// endpoints. All byte counts are in bytes.
struct FlowConsumeLog {
  int id = 0;
  int uid = 0;
  long long balance = 0;
  long long all_buy = 0;
  long long use = 0;
  long long day = 0;
  long long expire_flow = 0;
};
void from_json(const nlohmann::json& j, FlowConsumeLog& v);

// FlowConsumeLogList is the data payload of the residential, rotating ISP,
// rotating datacenter and mobile traffic-consumption endpoints.
struct FlowConsumeLogList {
  std::vector<FlowConsumeLog> list;
};

// --- Account -------------------------------------------------------------

// CreateAccountParams are the parameters for AccountService::create.
// product, account, password and status are required.
struct CreateAccountParams {
  Product product{};       // required; e.g. kResidential, kBrowserAPI
  std::string account;     // required; account name
  std::string password;    // required; account password
  int status = 0;          // required; 1=normal, -3=disabled
  std::string remark;      // optional remark
  std::string limit_flow;  // optional; dynamic residential data cap in GB
};

// ListAccountParams are the parameters for AccountService::list. product is
// required; page and limit default to 1 and 10 when left unset.
struct ListAccountParams {
  Product product{};          // required
  std::optional<int> status;  // optional filter; 1=normal, -3=disabled
  std::string account;        // optional account-name filter
  std::optional<int> page;    // page number (default 1)
  std::optional<int> limit;   // entries per page (default 10)
};

// UpdateAccountParams are the parameters for AccountService::update. id,
// account and password are required.
struct UpdateAccountParams {
  int id = 0;                 // required; account ID
  std::string account;        // required; account name
  std::string password;       // required; account password
  std::optional<int> status;  // optional; 1=normal, -3=disabled
  std::string remark;         // optional remark
  std::string limit_flow;     // optional residential data cap in GB
};

// ConsumeLogParams are the parameters for AccountService::consume_log.
// account_id is required; page and limit default to 1 and 10.
struct ConsumeLogParams {
  int account_id = 0;        // required
  std::string start_time;    // optional; "2006-01-02 15:04:05"
  std::string end_time;      // optional; "2006-01-02 15:04:05"
  std::optional<int> page;   // page number (default 1)
  std::optional<int> limit;  // entries per page (default 10)
};

// Account is a proxy sub-account as returned by AccountService::list.
struct Account {
  long long created_at = 0;
  long long updated_at = 0;
  int id = 0;
  int uid = 0;
  std::string account;
  std::string password;
  int status = 0;
  long long residential_balance = 0;
  long long residential_all_buy = 0;
  int residential_status = 0;
  long long dc_balance = 0;
  long long dc_all_buy = 0;
  int dc_status = 0;
  long long isp_balance = 0;
  long long isp_all_buy = 0;
  int isp_status = 0;
  std::string flow_type;
  std::string account_type;
  int check_white_list = 0;
  std::string remark;
  long long consumed_residential_flow = 0;
  long long limit_residential_flow = 0;
  long long consumed_dc_flow = 0;
  long long limit_dc_flow = 0;
  long long consumed_isp_flow = 0;
  long long limit_isp_flow = 0;
};
void from_json(const nlohmann::json& j, Account& v);

// AccountList is the data payload of AccountService::list.
struct AccountList {
  std::vector<Account> list;
  int page = 0;
  int total = 0;
};

// AccountConsumeLog is a single daily traffic-consumption record.
struct AccountConsumeLog {
  int id = 0;
  int account_id = 0;
  int uid = 0;
  long long residential_balance = 0;
  long long residential_all_buy = 0;
  long long consumed_residential_flow = 0;
  long long dc_balance = 0;
  long long dc_all_buy = 0;
  long long consumed_dc_flow = 0;
  long long isp_balance = 0;
  long long isp_all_buy = 0;
  long long consumed_isp_flow = 0;
  // day is the Unix timestamp (seconds) of the record's day.
  long long day = 0;
};
void from_json(const nlohmann::json& j, AccountConsumeLog& v);

// AccountConsumeLogList is the data payload of AccountService::consume_log.
struct AccountConsumeLogList {
  std::vector<AccountConsumeLog> list;
};

// --- Whitelist -----------------------------------------------------------

// AddWhitelistParams are the parameters for WhitelistService::add. product
// and ip are required. Valid products: 1=Residential, 5=Static ISP,
// 4=Unlimited.
struct AddWhitelistParams {
  Product product{};   // required
  std::string ip;      // required
  std::string remark;  // optional
};

// ListWhitelistParams are the parameters for WhitelistService::list. product
// is required; the remaining fields are optional filters.
struct ListWhitelistParams {
  Product product{};       // required
  std::string ip;          // optional IP filter
  std::string start_time;  // optional; "2006-01-02 15:04:05"
  std::string end_time;    // optional; "2006-01-02 15:04:05"
  // lock filters by lock state: 0=unlocked, 1=locked. nullopt means no filter.
  std::optional<int> lock;
};

// DeleteWhitelistParams are the parameters for WhitelistService::del.
// product and ips are required; pass multiple IPs comma-separated.
struct DeleteWhitelistParams {
  Product product{};  // required
  std::string ips;    // required; comma-separated list, e.g. "1.1.1.1,2.2.2.2"
};

// RemarkWhitelistParams are the parameters for WhitelistService::remark.
// product and id are required.
struct RemarkWhitelistParams {
  Product product{};   // required
  std::string id;      // required; the whitelist entry ID
  std::string remark;  // optional new remark
};

// WhitelistIP is a whitelist entry as returned by WhitelistService::list.
struct WhitelistIP {
  long long created_at = 0;
  long long updated_at = 0;
  int id = 0;
  int uid = 0;
  // mark_ip is the human-readable IP string (e.g. "10.10.10.1").
  std::string mark_ip;
  // ip is the integer form of the address as stored by the API.
  long long ip = 0;
  int status = 0;
  bool lock = false;
  int lock_uid = 0;
  int flag = 0;
  int is_biger = 0;
  int is_limit = 0;
  std::string mark;
};
void from_json(const nlohmann::json& j, WhitelistIP& v);

// WhitelistList is the data payload of WhitelistService::list.
struct WhitelistList {
  std::vector<WhitelistIP> list;
  int total = 0;
};

// --- Residential -----------------------------------------------------------

// CityParams selects a city listing by country code and region/state. Both
// fields are required.
struct CityParams {
  std::string code;    // required; country code, e.g. "us"
  std::string region;  // required; region/state name, e.g. "alabama"
};

// ResidentialCountry is a single residential proxy country.
struct ResidentialCountry {
  std::string code;
  int continent = 0;
  int ip_num = 0;
  std::string name;
  std::string name_en;
  int protocol = 0;
};
void from_json(const nlohmann::json& j, ResidentialCountry& v);

// ResidentialContinentGroup is a continent with its list of countries.
struct ResidentialContinentGroup {
  std::string continent;
  int continent_code = 0;
  std::vector<ResidentialCountry> list;
};
void from_json(const nlohmann::json& j, ResidentialContinentGroup& v);

// ResidentialAreas is the country listing returned by
// ResidentialService::countries. continent maps continent codes to names;
// country groups countries by continent.
struct ResidentialAreas {
  std::map<std::string, std::string> continent;
  std::vector<ResidentialContinentGroup> country;
};
void from_json(const nlohmann::json& j, ResidentialAreas& v);

// ResidentialState is a state/region within a country.
struct ResidentialState {
  std::string state;
};
void from_json(const nlohmann::json& j, ResidentialState& v);

// ResidentialStateList is the data payload of ResidentialService::states.
struct ResidentialStateList {
  std::vector<ResidentialState> data;
};

// ResidentialCity is a city within a region.
struct ResidentialCity {
  std::string code;
};
void from_json(const nlohmann::json& j, ResidentialCity& v);

// ResidentialCityList is the data payload of ResidentialService::cities.
struct ResidentialCityList {
  std::vector<ResidentialCity> data;
};

// ResidentialISP is an ISP available in a country.
struct ResidentialISP {
  std::string asn;
  std::string show_name;
};
void from_json(const nlohmann::json& j, ResidentialISP& v);

// ResidentialISPList is the data payload of ResidentialService::isps.
struct ResidentialISPList {
  std::vector<ResidentialISP> list;
};

// --- Mobile ------------------------------------------------------------

// MobileUseParams selects a mobile traffic window. start, end and
// granularity are required.
struct MobileUseParams {
  std::string start;  // required; "2006-01-02 15:04:05"
  std::string end;    // required; "2006-01-02 15:04:05"
  // granularity selects the bucket size: "1"=hour, "2"=day.
  std::string granularity;
};

// MobileBalance is the remaining mobile traffic in bytes.
struct MobileBalance {
  long long balance = 0;
};
void from_json(const nlohmann::json& j, MobileBalance& v);

// --- Rotating ISP / Rotating DC --------------------------------------------

// ISPCountry is a single rotating ISP country.
struct ISPCountry {
  std::string code;
  std::string node;
};
void from_json(const nlohmann::json& j, ISPCountry& v);

// ISPArea is the country listing returned by RotatingISPService::countries.
struct ISPArea {
  std::vector<ISPCountry> country;
};
void from_json(const nlohmann::json& j, ISPArea& v);

// DCCity is the city descriptor nested in a DCCountry.
struct DCCity {
  std::string code;
};
void from_json(const nlohmann::json& j, DCCity& v);

// DCCountry is a single rotating datacenter country.
struct DCCountry {
  int id = 0;
  std::string code;
  int status = 0;
  int continents = 0;
  int area_code = 0;
  std::string name_en;
  int available = 0;
  DCCity city;
};
void from_json(const nlohmann::json& j, DCCountry& v);

// DCArea is the country listing returned by RotatingDCService::countries.
struct DCArea {
  std::vector<DCCountry> country;
};
void from_json(const nlohmann::json& j, DCArea& v);

// --- Static (shared by StaticISP / DedicatedDC) -----------------------------

// StaticIP is a purchased static IP (ISP or dedicated datacenter) as returned
// by the list endpoints.
struct StaticIP {
  int id = 0;
  std::string ip;
  std::string port;
  long long duration = 0;
  std::string region;
  long long expire_time = 0;
  long long create_time = 0;
  std::string country;
  std::string country_en;
  int status = 0;
  std::string remark;
  int auto_renew = 0;
  int type = 0;
  int diff_day = 0;
  std::string account;
  std::string password;
};
void from_json(const nlohmann::json& j, StaticIP& v);

// StaticIPList is the data payload of the static IP list endpoints.
struct StaticIPList {
  std::vector<StaticIP> list;
  int page = 0;
  int total = 0;
};

// RegionNode is a single selectable region in the region listings.
struct RegionNode {
  std::string node;
  std::string node_en;
  int param = 0;
  std::string region;
};
void from_json(const nlohmann::json& j, RegionNode& v);

// RegionList is the data payload of the static region endpoints; regions are
// grouped by area name (e.g. "Asia-Pacific").
struct RegionList {
  std::map<std::string, std::vector<RegionNode>> list;
};
void from_json(const nlohmann::json& j, RegionList& v);

// ListStaticParams are the filters for the static IP list endpoints. page and
// limit default to 1 and 10 when left unset.
struct ListStaticParams {
  std::string status;    // "" = all, "1" = in use, "2" = expired, "3" = released
  std::string region;    // optional area code
  std::string key_word;  // optional keyword (remark, order number, IP)
  // is_auto_renew filters by auto-renew flag (1 = yes, -1 = no). nullopt = no
  // filter.
  std::optional<int> is_auto_renew;
  std::optional<int> page;
  std::optional<int> limit;
};

// ExportStaticParams are the filters for the static IP export endpoints.
struct ExportStaticParams {
  std::string status;
  std::string region;
  std::string key_word;
  std::optional<int> is_auto_renew;
};

// RenewStaticParams renews one or more static IPs. Both fields are required.
struct RenewStaticParams {
  // ips is a comma-separated list of IPs, e.g. "1.1.1.1,2.2.2.2".
  std::string ips;
  // duration is the activation period: "week" or "month".
  std::string duration;
};

// RenewSettingParams updates the auto-renewal settings of static IPs. All
// fields are required; the product type is supplied automatically by the
// service method.
struct RenewSettingParams {
  // ids is a comma-separated list of IP IDs, e.g. "100,111,112".
  std::string ids;
  // package_type is the activation period: "week" or "month".
  std::string package_type;
  // status is the renewal status: 1 = normal, -1 = disabled.
  int status = 0;
  // renew_type is the renewal method: 1 = wallet, 2 = credit card.
  int renew_type = 0;
};

// OpenStaticISPParams opens static ISP IPs. All fields are required.
struct OpenStaticISPParams {
  // ip_type is the IP grade: "normal" = standard, "premium" = premium.
  std::string ip_type;
  // region is an "area:num" spec, e.g. "hk:1|us-va:2".
  std::string region;
  // duration is the activation period: "week" or "month".
  std::string duration;
  // num is the number of IPs to open.
  int num = 0;
};

// OpenDedicatedDCParams opens dedicated datacenter IPs. All fields are
// required.
struct OpenDedicatedDCParams {
  // region is an "area:num" spec, e.g. "hk:1|tw:2".
  std::string region;
  // duration is the activation period: "week" or "month".
  std::string duration;
  // num is the number of IPs to open.
  int num = 0;
};

// --- Unlimited -----------------------------------------------------------

// UnlimitedHost is a single unlimited proxy server.
struct UnlimitedHost {
  int id = 0;
  long long create_time = 0;
  long long expire_time = 0;
  int state = 0;
  std::string host;
  long long user_rate_limit_bytes = 0;
  std::string order_id;
  int days = 0;
  int duration = 0;
  long long band_width = 0;
  std::string hardware;
};
void from_json(const nlohmann::json& j, UnlimitedHost& v);

// UnlimitedHostList is the data payload of UnlimitedService::hosts.
struct UnlimitedHostList {
  std::vector<UnlimitedHost> list;
  int page = 0;
  int total = 0;
};

// --- ProhibitDomain --------------------------------------------------------

// ProhibitDomain is a single blocked-domain entry.
struct ProhibitDomain {
  long long created_at = 0;
  long long updated_at = 0;
  int id = 0;
  int uid = 0;
  int status = 0;
  std::string address;
};
void from_json(const nlohmann::json& j, ProhibitDomain& v);

// ProhibitDomainList is the data payload of ProhibitDomainService::list.
struct ProhibitDomainList {
  std::vector<ProhibitDomain> list;
  int total = 0;
};

// DeleteProhibitParams identifies which blocked domains to delete. When all
// is true every entry is deleted and id is ignored; otherwise id is required.
struct DeleteProhibitParams {
  std::string id;    // entry ID; required unless all is true
  bool all = false;  // delete all entries
};

}  // namespace novada::proxy
