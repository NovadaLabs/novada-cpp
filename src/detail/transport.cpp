#include "novada/detail/transport.hpp"

#include <curl/curl.h>

#include <mutex>
#include <thread>

#include "novada/detail/envelope.hpp"
#include "novada/errors.hpp"

namespace novada::detail {

namespace {

// retryBaseDelay is the base backoff between retry attempts; it is multiplied
// by the attempt index to produce a simple linear backoff, matching the Go
// SDK's retryBaseDelay.
constexpr std::chrono::milliseconds kRetryBaseDelay{200};

void ensure_curl_global_init() {
  static std::once_flag once;
  std::call_once(once, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

// CurlHandle RAII-wraps a libcurl easy handle. A fresh handle is created for
// every request attempt so Transport holds no shared mutable curl state.
class CurlHandle {
 public:
  CurlHandle() : handle_(curl_easy_init()) {
    if (handle_ == nullptr) {
      throw NovadaException("novada: curl_easy_init failed");
    }
  }
  ~CurlHandle() {
    if (handle_ != nullptr) {
      curl_easy_cleanup(handle_);
    }
  }
  CurlHandle(const CurlHandle&) = delete;
  CurlHandle& operator=(const CurlHandle&) = delete;
  CurlHandle(CurlHandle&&) = delete;
  CurlHandle& operator=(CurlHandle&&) = delete;

  [[nodiscard]] CURL* get() const noexcept { return handle_; }

 private:
  CURL* handle_;
};

// CurlSlist RAII-wraps a libcurl string list (used for request headers).
class CurlSlist {
 public:
  CurlSlist() = default;
  ~CurlSlist() {
    if (list_ != nullptr) {
      curl_slist_free_all(list_);
    }
  }
  CurlSlist(const CurlSlist&) = delete;
  CurlSlist& operator=(const CurlSlist&) = delete;
  CurlSlist(CurlSlist&&) = delete;
  CurlSlist& operator=(CurlSlist&&) = delete;

  void append(const std::string& header) { list_ = curl_slist_append(list_, header.c_str()); }
  [[nodiscard]] curl_slist* get() const noexcept { return list_; }

 private:
  curl_slist* list_ = nullptr;
};

// CurlMime RAII-wraps a libcurl mime structure used to build
// multipart/form-data bodies. curl_mime_data() copies field values into
// libcurl's own storage, so the source strings need not outlive this object.
class CurlMime {
 public:
  explicit CurlMime(CURL* curl) : mime_(curl_mime_init(curl)) {
    if (mime_ == nullptr) {
      throw NovadaException("novada: curl_mime_init failed");
    }
  }
  ~CurlMime() {
    if (mime_ != nullptr) {
      curl_mime_free(mime_);
    }
  }
  CurlMime(const CurlMime&) = delete;
  CurlMime& operator=(const CurlMime&) = delete;
  CurlMime(CurlMime&&) = delete;
  CurlMime& operator=(CurlMime&&) = delete;

  [[nodiscard]] curl_mime* get() const noexcept { return mime_; }

 private:
  curl_mime* mime_;
};

std::size_t write_callback(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
  auto* out = static_cast<std::string*>(userdata);
  out->append(ptr, size * nmemb);
  return size * nmemb;
}

// http_error_message extracts a human-readable message for a non-2xx
// response, preferring the envelope "msg" field when the body parses,
// otherwise falling back to a generic HTTP status message.
std::string http_error_message(const std::string& body, long http_status) {
  try {
    nlohmann::json parsed = nlohmann::json::parse(body);
    std::string msg = parsed.value("msg", std::string());
    if (!msg.empty()) {
      return msg;
    }
  } catch (const nlohmann::json::parse_error&) {  // NOLINT(bugprone-empty-catch)
    // Fall through to the generic status message below.
  }
  return "HTTP " + std::to_string(http_status);
}

// setup_common applies the options shared by every request: URL, POST
// method, timeout, write buffer and headers (Authorization/Accept/
// User-Agent, plus an optional explicit Content-Type; multipart requests
// leave content_type empty so libcurl derives it from the mime boundary).
void setup_common(CURL* curl, const std::string& full_url, const std::string& api_key,
                  const std::string& content_type, const std::string& user_agent,
                  std::chrono::milliseconds timeout, CurlSlist& headers,
                  std::string& response_body) {
  curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(timeout.count()));
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());

  headers.append("Authorization: Bearer " + api_key);
  headers.append("Accept: application/json");
  if (!content_type.empty()) {
    headers.append("Content-Type: " + content_type);
  }
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers.get());
}

}  // namespace

std::string join_url(const std::string& base, const std::string& path) {
  std::string trimmed_base = base;
  while (!trimmed_base.empty() && trimmed_base.back() == '/') {
    trimmed_base.pop_back();
  }
  if (!path.empty() && path.front() == '/') {
    return trimmed_base + path;
  }
  return trimmed_base + "/" + path;
}

Transport::Transport(std::string api_key, std::string base_url, std::string web_unblocker_url,
                     std::string scraper_url, std::chrono::milliseconds timeout, int max_retries,
                     std::string user_agent)
    : api_key_(std::move(api_key)),
      base_url_(std::move(base_url)),
      web_unblocker_url_(std::move(web_unblocker_url)),
      scraper_url_(std::move(scraper_url)),
      timeout_(timeout),
      max_retries_(max_retries),
      user_agent_(std::move(user_agent)) {
  ensure_curl_global_init();
}

Transport::RawResponse Transport::post_multipart(
    const std::string& full_url, const std::map<std::string, std::string>& fields) const {
  std::string last_network_error;
  bool have_retryable_response = false;
  long retryable_status = 0;
  std::string retryable_body;

  for (int attempt = 0; attempt <= max_retries_; ++attempt) {
    if (attempt > 0) {
      std::this_thread::sleep_for(kRetryBaseDelay * attempt);
    }

    CurlHandle handle;
    CURL* curl = handle.get();
    std::string response_body;
    CurlSlist headers;
    // Multipart's Content-Type (with boundary) is set by libcurl itself from
    // the mime structure below, so no explicit header here.
    setup_common(curl, full_url, api_key_, /*content_type=*/"", user_agent_, timeout_, headers,
                 response_body);

    CurlMime mime(curl);
    for (const auto& [key, value] : fields) {
      if (value.empty()) {
        continue;  // omit empty fields, matching the Go SDK's semantics
      }
      curl_mimepart* part = curl_mime_addpart(mime.get());
      curl_mime_name(part, key.c_str());
      curl_mime_data(part, value.c_str(), value.size());
    }
    // When fields is entirely empty (e.g. wallet.Balance(), which sends no
    // fields at all), curl_mime with zero parts serializes to just the
    // closing boundary line ("--BOUNDARY--\r\n"), matching the Go SDK's
    // mime/multipart.Writer.Close() behavior with nothing written. This is a
    // valid (if degenerate) multipart/form-data body per RFC 2046.
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime.get());

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      last_network_error = curl_easy_strerror(res);
      have_retryable_response = false;
      continue;  // network error: retry
    }

    long http_status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);

    if (http_status == 429 || http_status >= 500) {
      have_retryable_response = true;
      retryable_status = http_status;
      retryable_body = response_body;
      continue;  // 429/5xx: retry
    }

    if (http_status < 200 || http_status >= 300) {
      detail::throw_api_exception(static_cast<int>(http_status), 0,
                                  http_error_message(response_body, http_status));
    }

    return RawResponse{std::move(response_body), http_status};
  }

  if (have_retryable_response) {
    detail::throw_api_exception(static_cast<int>(retryable_status), 0,
                                http_error_message(retryable_body, retryable_status));
  }
  throw NovadaException("novada: request failed: " + last_network_error);
}

Transport::RawResponse Transport::post_urlencoded(
    const std::string& full_url, const std::map<std::string, std::string>& fields) const {
  std::string last_network_error;
  bool have_retryable_response = false;
  long retryable_status = 0;
  std::string retryable_body;

  // The encoded body is identical across retry attempts; curl_easy_escape
  // needs a valid handle, so build it once against a throwaway handle.
  std::string encoded_body;
  {
    CurlHandle escape_handle;
    bool first = true;
    for (const auto& [key, value] : fields) {
      if (value.empty()) {
        continue;
      }
      char* escaped_key =
          curl_easy_escape(escape_handle.get(), key.c_str(), static_cast<int>(key.size()));
      char* escaped_value =
          curl_easy_escape(escape_handle.get(), value.c_str(), static_cast<int>(value.size()));
      if (!first) {
        encoded_body += '&';
      }
      first = false;
      encoded_body += escaped_key;
      encoded_body += '=';
      encoded_body += escaped_value;
      curl_free(escaped_key);
      curl_free(escaped_value);
    }
  }

  for (int attempt = 0; attempt <= max_retries_; ++attempt) {
    if (attempt > 0) {
      std::this_thread::sleep_for(kRetryBaseDelay * attempt);
    }

    CurlHandle handle;
    CURL* curl = handle.get();
    std::string response_body;
    CurlSlist headers;
    setup_common(curl, full_url, api_key_, "application/x-www-form-urlencoded", user_agent_,
                 timeout_, headers, response_body);

    // CURLOPT_POSTFIELDSIZE must be set before CURLOPT_COPYPOSTFIELDS so
    // libcurl knows how much to copy up front instead of falling back to a
    // read-callback-based body (which would fail with CURLE_READ_ERROR since
    // no CURLOPT_READFUNCTION is installed). CURLOPT_COPYPOSTFIELDS then
    // copies encoded_body into libcurl's own buffer, so it need not outlive
    // this call.
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(encoded_body.size()));
    curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, encoded_body.c_str());

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      last_network_error = curl_easy_strerror(res);
      have_retryable_response = false;
      continue;  // network error: retry
    }

    long http_status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);

    if (http_status == 429 || http_status >= 500) {
      have_retryable_response = true;
      retryable_status = http_status;
      retryable_body = response_body;
      continue;  // 429/5xx: retry
    }

    if (http_status < 200 || http_status >= 300) {
      detail::throw_api_exception(static_cast<int>(http_status), 0,
                                  http_error_message(response_body, http_status));
    }

    return RawResponse{std::move(response_body), http_status};
  }

  if (have_retryable_response) {
    detail::throw_api_exception(static_cast<int>(retryable_status), 0,
                                http_error_message(retryable_body, retryable_status));
  }
  throw NovadaException("novada: request failed: " + last_network_error);
}

nlohmann::json Transport::do_multipart(const std::string& base_url, const std::string& path,
                                       const std::map<std::string, std::string>& fields) const {
  RawResponse resp = post_multipart(join_url(base_url, path), fields);
  return decode_envelope(static_cast<int>(resp.http_status), resp.body);
}

std::string Transport::do_multipart_raw(const std::string& base_url, const std::string& path,
                                        const std::map<std::string, std::string>& fields) const {
  return post_multipart(join_url(base_url, path), fields).body;
}

nlohmann::json Transport::do_form_urlencoded(
    const std::string& base_url, const std::string& path,
    const std::map<std::string, std::string>& fields) const {
  RawResponse resp = post_urlencoded(join_url(base_url, path), fields);
  return decode_envelope(static_cast<int>(resp.http_status), resp.body);
}

std::string Transport::do_form_urlencoded_raw(
    const std::string& base_url, const std::string& path,
    const std::map<std::string, std::string>& fields) const {
  return post_urlencoded(join_url(base_url, path), fields).body;
}

}  // namespace novada::detail
