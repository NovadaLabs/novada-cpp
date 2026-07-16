#pragma once

#include <chrono>
#include <memory>
#include <string>

#include "novada/version.hpp"

namespace novada::detail {
class Transport;
}  // namespace novada::detail

namespace novada::proxy {
class ProxyService;
}  // namespace novada::proxy

namespace novada::scraper {
class ScraperService;
}  // namespace novada::scraper

namespace novada::wallet {
class WalletService;
}  // namespace novada::wallet

namespace novada {

// Default base URLs and tuning values. All three hosts can be overridden via
// the corresponding ClientOptions setter; the defaults target Novada's public
// production endpoints.

// kDefaultBaseUrl serves every /v1/* management endpoint (proxy, wallet, and
// the scraper query endpoints such as unblocker_area or capture/get_balance).
inline constexpr const char* kDefaultBaseUrl = "https://api-m.novada.com";
// kDefaultWebUnblockerUrl serves the Web Unblocker POST /request endpoint.
inline constexpr const char* kDefaultWebUnblockerUrl = "https://webunlocker.novada.com";
// kDefaultScraperUrl serves the Scraper API POST /request endpoint.
inline constexpr const char* kDefaultScraperUrl = "https://scraper.novada.com";

inline constexpr std::chrono::milliseconds kDefaultTimeout{30000};
inline constexpr int kDefaultMaxRetries = 2;

// ClientOptions configures a Client. It is a plain aggregate with default
// member initializers (the C++ analogue of the Go SDK's functional Options);
// each field also has a chainable setter for fluent construction:
//
//   novada::ClientOptions opts;
//   opts.set_base_url("...").set_timeout(std::chrono::seconds(30));
//   novada::Client client("API_KEY", opts);
class ClientOptions {
 public:
  std::string base_url = kDefaultBaseUrl;
  std::string web_unblocker_url = kDefaultWebUnblockerUrl;
  std::string scraper_url = kDefaultScraperUrl;
  std::chrono::milliseconds timeout = kDefaultTimeout;
  int max_retries = kDefaultMaxRetries;
  std::string user_agent = std::string("novada-cpp/") + kVersion;

  // set_base_url overrides the general host used for all /v1/* endpoints.
  ClientOptions& set_base_url(std::string value) {
    base_url = std::move(value);
    return *this;
  }

  // set_web_unblocker_url overrides the host used for the Web Unblocker
  // POST /request endpoint.
  ClientOptions& set_web_unblocker_url(std::string value) {
    web_unblocker_url = std::move(value);
    return *this;
  }

  // set_scraper_url overrides the host used for the Scraper API
  // POST /request endpoint.
  ClientOptions& set_scraper_url(std::string value) {
    scraper_url = std::move(value);
    return *this;
  }

  // set_timeout sets the per-request timeout.
  ClientOptions& set_timeout(std::chrono::milliseconds value) {
    timeout = value;
    return *this;
  }

  // set_max_retries sets how many times a request is retried on network
  // errors or HTTP 429/5xx responses. Business failures (code != 0) are
  // never retried.
  ClientOptions& set_max_retries(int value) {
    max_retries = value;
    return *this;
  }

  // set_user_agent overrides the User-Agent header sent on every request.
  ClientOptions& set_user_agent(std::string value) {
    user_agent = std::move(value);
    return *this;
  }
};

// Client is the top-level Novada API client. It injects the Bearer API key on
// every request and routes calls to one of three hosts depending on the
// endpoint. It is safe for concurrent use by multiple threads: each request
// creates its own libcurl easy handle, so no mutable state is shared across
// requests.
//
// Sub-services (proxy, scraper, wallet) are reached through accessor
// methods, e.g. client.proxy().whitelist().list(...).
//
// The api_key argument takes precedence; when it is empty the NOVADA_API_KEY
// environment variable is used as a fallback. Throws NovadaException when
// neither is set.
class Client {
 public:
  explicit Client(const std::string& api_key = "", ClientOptions options = ClientOptions{});
  ~Client();

  Client(Client&&) noexcept;
  Client& operator=(Client&&) noexcept;
  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;

  // base_url returns the general host used for /v1/* endpoints.
  [[nodiscard]] const std::string& base_url() const noexcept;
  // web_unblocker_url returns the host used for the Web Unblocker
  // POST /request endpoint.
  [[nodiscard]] const std::string& web_unblocker_url() const noexcept;
  // scraper_url returns the host used for the Scraper API POST /request
  // endpoint.
  [[nodiscard]] const std::string& scraper_url() const noexcept;

  // proxy exposes the proxy management endpoints (/v1/* multipart APIs).
  [[nodiscard]] const proxy::ProxyService& proxy() const noexcept;

  // scraper exposes the scraping endpoints (scrape /request plus queries).
  [[nodiscard]] const scraper::ScraperService& scraper() const noexcept;

  // wallet exposes the wallet endpoints (/v1/wallet/*).
  [[nodiscard]] const wallet::WalletService& wallet() const noexcept;

 private:
  std::unique_ptr<detail::Transport> transport_;
  std::unique_ptr<proxy::ProxyService> proxy_;
  std::unique_ptr<scraper::ScraperService> scraper_;
  std::unique_ptr<wallet::WalletService> wallet_;
};

}  // namespace novada
