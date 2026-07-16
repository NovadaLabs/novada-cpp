#include "novada/client.hpp"

#include <cstdlib>

#include "novada/detail/transport.hpp"
#include "novada/errors.hpp"
#include "novada/proxy/proxy_service.hpp"
#include "novada/scraper/scraper_service.hpp"
#include "novada/wallet/wallet_service.hpp"

namespace novada {

namespace {

std::string resolve_api_key(const std::string& api_key) {
  if (!api_key.empty()) {
    return api_key;
  }
  const char* env = std::getenv("NOVADA_API_KEY");
  if (env != nullptr && *env != '\0') {
    return env;
  }
  throw NovadaException("novada: API key is required (pass it to Client() or set NOVADA_API_KEY)");
}

}  // namespace

Client::Client(const std::string& api_key, ClientOptions options)
    : transport_(std::make_unique<detail::Transport>(
          resolve_api_key(api_key), std::move(options.base_url),
          std::move(options.web_unblocker_url), std::move(options.scraper_url), options.timeout,
          options.max_retries, std::move(options.user_agent))),
      proxy_(std::make_unique<proxy::ProxyService>(*transport_)),
      scraper_(std::make_unique<scraper::ScraperService>(*transport_)),
      wallet_(std::make_unique<wallet::WalletService>(*transport_)) {}

Client::~Client() = default;
Client::Client(Client&&) noexcept = default;
Client& Client::operator=(Client&&) noexcept = default;

const std::string& Client::base_url() const noexcept {
  return transport_->base_url();
}
const std::string& Client::web_unblocker_url() const noexcept {
  return transport_->web_unblocker_url();
}
const std::string& Client::scraper_url() const noexcept {
  return transport_->scraper_url();
}
const proxy::ProxyService& Client::proxy() const noexcept {
  return *proxy_;
}
const scraper::ScraperService& Client::scraper() const noexcept {
  return *scraper_;
}
const wallet::WalletService& Client::wallet() const noexcept {
  return *wallet_;
}

}  // namespace novada
