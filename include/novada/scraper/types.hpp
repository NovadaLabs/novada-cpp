#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace novada::scraper {

// --- YouTube -----------------------------------------------------------

// YouTubeVideoParams are the parameters for YouTubeService::video_post.
struct YouTubeVideoParams {
  // url is the YouTube video URL. Required.
  std::string url;
};

// --- Google (SerpApi) ------------------------------------------------------

// GoogleSearchParams configures a Google Search scrape (scraper_id
// "google_search"). query is required; optional fields are sent only when set.
struct GoogleSearchParams {
  // query is the search query or URL. Required (q).
  std::string query;
  // device emulates a device type: "desktop", "tablet" or "mobile".
  std::string device;
  // html requests HTML output (json=0) instead of the default JSON output
  // (json=1).
  bool html = false;
  // render_js enables JavaScript rendering (render_js).
  std::optional<bool> render_js;
  // no_cache skips the cache and forces a real-time request (no_cache).
  std::optional<bool> no_cache;
  // ai_overview fetches the AI Overview block.
  std::optional<bool> ai_overview;
  // domain is the Google domain to crawl, e.g. "google.com".
  std::string domain;
  // country is the two-letter country/region code (gl); server default "us".
  std::string country;
  // hl is the results language (hl).
  std::string hl;
  // return_errors sets scraper_errors=true so the response includes scrape
  // errors.
  bool return_errors = false;
};

// GoogleSearchData holds the scraped output. json is the raw result array (its
// structure is large and varies by query, so it is left as raw JSON text);
// html is set instead when HTML output was requested.
struct GoogleSearchData {
  std::string filename;
  std::optional<std::string> html;
  // json is the raw JSON text of the scraped result array (empty when HTML
  // output was requested).
  std::string json;
  std::string task_id;
};
void from_json(const nlohmann::json& j, GoogleSearchData& v);

// GoogleSearchResult is the decoded "data" payload of a Google Search scrape.
struct GoogleSearchResult {
  // code is the page-level status (200 on success).
  int code = 0;
  // cost_time is the scrape duration in milliseconds.
  int cost_time = 0;
  // msg is a short status message.
  std::string msg;
  // data holds the scraped output.
  GoogleSearchData data;
};
void from_json(const nlohmann::json& j, GoogleSearchResult& v);

// --- Web Unblocker ------------------------------------------------------

// UnblockerParams configures a Web Unblocker scrape (POST /request). Only
// target_url is required; response_format defaults to "html" when empty, and
// the remaining optional fields are sent only when set.
struct UnblockerParams {
  // target_url is the destination address to scrape. Required.
  std::string target_url;
  // response_format is the output format: "html", "png" or "html,png".
  // Defaults to "html" when empty.
  std::string response_format;
  // js_render enables JS rendering to capture dynamically loaded content.
  std::optional<bool> js_render;
  // headers are custom request headers used to access the site.
  std::string headers;
  // cookies are custom cookies used to access the site.
  std::string cookies;
  // country is the proxy country/region, e.g. "us".
  std::string country;
  // wait_ms is the maximum page wait time in milliseconds (max 100000). Sent
  // only when positive.
  int wait_ms = 0;
  // wait_selector waits for a CSS selector to appear in the DOM (max 30s). It
  // takes precedence over wait_ms when both are set.
  std::string wait_selector;
  // follow_redirects follows redirects from expired URLs to their new target.
  std::optional<bool> follow_redirects;
  // block_resources skips loading images, JS files and videos to speed up the
  // crawl.
  std::optional<bool> block_resources;
  // clear strips unnecessary JS and CSS from the crawl result.
  std::optional<bool> clear;
  // auto_runs is the number of automatic retries on proxy failure (0-10,
  // server default 2). Sent only when engaged.
  std::optional<int> auto_runs;
};

// UnblockerResult is the decoded data payload of a Web Unblocker scrape.
struct UnblockerResult {
  // code is the page-level status (200 on success).
  int code = 0;
  // html is the scraped content.
  std::string html;
  // msg is a short status message.
  std::string msg;
  // msg_detail carries additional error detail when the scrape fails.
  std::string msg_detail;
  // use_balance is the balance consumed by this call.
  double use_balance = 0;
};
void from_json(const nlohmann::json& j, UnblockerResult& v);

// --- Universal (capture queries) -------------------------------------------

// ScraperBalance is the remaining scraper balance.
struct ScraperBalance {
  double scraper_balance = 0;
};
void from_json(const nlohmann::json& j, ScraperBalance& v);

// UsageLog is a single daily scraper/unblocker/browser consumption record.
struct UsageLog {
  std::string time_label;
  double unlocker_total_cost = 0;
  double scraper_total_cost = 0;
  long long unlocker_used_res = 0;
  long long scraper_used_res = 0;
  long long scraper_used_flow = 0;
  double browser_total_cost = 0;
  long long browser_used_flow = 0;
};
void from_json(const nlohmann::json& j, UsageLog& v);

// UsageLogList is the data payload of UniversalService::logs.
struct UsageLogList {
  std::vector<UsageLog> list;
};

// UnitPrice is a single price tier for a scraper package.
struct UnitPrice {
  std::string package;
  int level = 0;
  double price = 0;
  int available = 0;
};
void from_json(const nlohmann::json& j, UnitPrice& v);

// UnitPrices is the data payload of UniversalService::unit, grouping prices by
// product.
struct UnitPrices {
  std::vector<UnitPrice> scraper;
  std::vector<UnitPrice> unblocker;
};
void from_json(const nlohmann::json& j, UnitPrices& v);

// --- Areas (shared shapes) -------------------------------------------------

// Country is a single country in an area listing.
struct Country {
  std::string code;
  int continent = 0;
  int ip_num = 0;
  std::string name;
  std::string name_en;
  int protocol = 0;
};
void from_json(const nlohmann::json& j, Country& v);

// ContinentGroup is a continent with its countries.
struct ContinentGroup {
  std::string continent;
  int continent_code = 0;
  std::vector<Country> list;
};
void from_json(const nlohmann::json& j, ContinentGroup& v);

// Areas is a country listing grouped by continent, shared by the unblocker
// area endpoint and the residential-style listings.
struct Areas {
  std::map<std::string, std::string> continent;
  std::vector<ContinentGroup> country;
};
void from_json(const nlohmann::json& j, Areas& v);

// State is a state/region within a country.
struct State {
  std::string state;
};
void from_json(const nlohmann::json& j, State& v);

// StateList is the data payload of the state listings.
struct StateList {
  std::vector<State> data;
};

// City is a city within a region.
struct City {
  std::string code;
};
void from_json(const nlohmann::json& j, City& v);

// CityList is the data payload of the city listings.
struct CityList {
  std::vector<City> data;
};

// ISP is a carrier available in a country.
struct ISP {
  std::string asn;
  std::string show_name;
};
void from_json(const nlohmann::json& j, ISP& v);

// ISPList is the data payload of the ISP listings.
struct ISPList {
  std::vector<ISP> list;
};

// --- Browser API -----------------------------------------------------------

// BrowserFlowUse is a single Browser API traffic-consumption record.
struct BrowserFlowUse {
  int id = 0;
  int uid = 0;
  long long balance = 0;
  long long all_buy = 0;
  long long use = 0;
  long long day = 0;
  long long expire_flow = 0;
};
void from_json(const nlohmann::json& j, BrowserFlowUse& v);

// BrowserFlowUseList is the data payload of BrowserService::flow_use.
struct BrowserFlowUseList {
  std::vector<BrowserFlowUse> list;
};

}  // namespace novada::scraper
