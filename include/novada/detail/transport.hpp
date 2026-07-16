#pragma once

#include <chrono>
#include <map>
#include <string>

#include <nlohmann/json.hpp>

namespace novada::detail {

// join_url concatenates a base URL and a path, tolerating a trailing slash on
// the base and a missing leading slash on the path.
std::string join_url(const std::string& base, const std::string& path);

// Transport performs the two request encodings used by the Novada API
// (multipart/form-data for /v1/* management endpoints, x-www-form-urlencoded
// for the scraper /request endpoints), injects the Bearer API key, and
// applies linear-backoff retries for transport-layer errors and HTTP
// 429/5xx responses. Business failures (envelope code() != 0) are never
// retried.
//
// It also owns the three resolved base URLs (general / Web Unblocker /
// Scraper API), mirroring the Go SDK's internal transport.Doer interface so
// sub-services can be constructed against a single reference without
// depending on the top-level Client type.
//
// Each request creates its own libcurl easy handle (see detail/curl_raii.hpp
// in transport.cpp), so a Transport instance holds no per-request mutable
// state and is safe to share across threads, mirroring Go's *http.Client
// reuse pattern.
class Transport {
 public:
  Transport(std::string api_key, std::string base_url, std::string web_unblocker_url,
            std::string scraper_url, std::chrono::milliseconds timeout, int max_retries,
            std::string user_agent);

  // do_multipart sends a multipart/form-data POST to base_url+path (the
  // encoding used by every /v1/* management endpoint) and returns the
  // decoded "data" field of the response envelope. Fields with an empty
  // value are omitted, matching the Go SDK's zero-value-omission semantics.
  nlohmann::json do_multipart(const std::string& base_url, const std::string& path,
                              const std::map<std::string, std::string>& fields) const;

  // do_multipart_raw is like do_multipart but returns the raw response body
  // without envelope decoding, for endpoints that return a file stream (e.g.
  // the static IP export endpoints) rather than the standard JSON envelope.
  std::string do_multipart_raw(const std::string& base_url, const std::string& path,
                               const std::map<std::string, std::string>& fields) const;

  // do_form_urlencoded sends an application/x-www-form-urlencoded POST to
  // base_url+path (the encoding used by the scraper /request endpoints) and
  // returns the decoded "data" field of the response envelope.
  nlohmann::json do_form_urlencoded(const std::string& base_url, const std::string& path,
                                    const std::map<std::string, std::string>& fields) const;

  // do_form_urlencoded_raw is like do_form_urlencoded but returns the raw
  // response body without envelope decoding. Scraper /request responses are
  // the scraped payload itself (JSON, CSV or XLSX) rather than the
  // management envelope.
  std::string do_form_urlencoded_raw(const std::string& base_url, const std::string& path,
                                     const std::map<std::string, std::string>& fields) const;

  // base_url returns the general host for /v1/* endpoints.
  [[nodiscard]] const std::string& base_url() const noexcept { return base_url_; }
  // web_unblocker_url returns the Web Unblocker host.
  [[nodiscard]] const std::string& web_unblocker_url() const noexcept { return web_unblocker_url_; }
  // scraper_url returns the Scraper API host.
  [[nodiscard]] const std::string& scraper_url() const noexcept { return scraper_url_; }

 private:
  struct RawResponse {
    std::string body;
    long http_status = 0;
  };

  RawResponse post_multipart(const std::string& full_url,
                             const std::map<std::string, std::string>& fields) const;
  RawResponse post_urlencoded(const std::string& full_url,
                              const std::map<std::string, std::string>& fields) const;

  std::string api_key_;
  std::string base_url_;
  std::string web_unblocker_url_;
  std::string scraper_url_;
  std::chrono::milliseconds timeout_;
  int max_retries_;
  std::string user_agent_;
};

}  // namespace novada::detail
