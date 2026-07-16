#pragma once

#include <stdexcept>
#include <string>
#include <vector>

namespace novada {

// NovadaException is the base class for every exception raised by this SDK.
// Catching it catches both API-level failures (ApiException and its
// subclasses) and client-side validation failures (ValidationException).
class NovadaException : public std::runtime_error {
 public:
  explicit NovadaException(const std::string& message) : std::runtime_error(message) {}
};

// ApiException represents a failed Novada API call. It is thrown in two
// situations:
//   - the HTTP response status was outside the 2xx range, in which case
//     http_status() is set and code() is 0; or
//   - the HTTP status was 2xx but the response envelope carried a non-zero
//     business code, in which case both http_status() and code() are set.
//
// Prefer is_auth_error()/is_rate_limited() (or catching the AuthException /
// RateLimitException subclasses directly) over inspecting http_status()
// yourself.
class ApiException : public NovadaException {
 public:
  ApiException(int http_status, int code, std::string message)
      : NovadaException(format_message(http_status, code, message)),
        http_status_(http_status),
        code_(code),
        message_(std::move(message)) {}

  // http_status returns the HTTP status code of the response.
  [[nodiscard]] int http_status() const noexcept { return http_status_; }

  // code returns the Novada business code carried in the response envelope.
  // It is 0 when the failure occurred at the HTTP layer (e.g. a 5xx without a
  // parseable envelope) rather than the business layer.
  [[nodiscard]] int code() const noexcept { return code_; }

  // message returns the human-readable error message (the envelope "msg"
  // field when available).
  [[nodiscard]] const std::string& message() const noexcept { return message_; }

 private:
  static std::string format_message(int http_status, int code, const std::string& message);

  int http_status_;
  int code_;
  std::string message_;
};

// AuthException is an ApiException caused by an authentication or
// authorization failure (HTTP 401 or 403).
class AuthException : public ApiException {
 public:
  using ApiException::ApiException;
};

// RateLimitException is an ApiException caused by rate limiting (HTTP 429).
class RateLimitException : public ApiException {
 public:
  using ApiException::ApiException;
};

// ValidationException reports that one or more required parameters were
// missing, detected client-side before any request was sent.
class ValidationException : public NovadaException {
 public:
  ValidationException(std::string method, std::vector<std::string> fields)
      : NovadaException(format_message(method, fields)),
        method_(std::move(method)),
        fields_(std::move(fields)) {}

  // method returns the SDK method that performed the validation, e.g.
  // "Whitelist.List".
  [[nodiscard]] const std::string& method() const noexcept { return method_; }

  // fields lists the names of the missing required parameters.
  [[nodiscard]] const std::vector<std::string>& fields() const noexcept { return fields_; }

 private:
  static std::string format_message(const std::string& method,
                                    const std::vector<std::string>& fields);

  std::string method_;
  std::vector<std::string> fields_;
};

// is_auth_error reports whether e was caused by an authentication or
// authorization failure (HTTP 401 or 403).
bool is_auth_error(const ApiException& e) noexcept;

// is_rate_limited reports whether e was caused by rate limiting (HTTP 429).
bool is_rate_limited(const ApiException& e) noexcept;

namespace detail {

// throw_api_exception selects AuthException (401/403), RateLimitException
// (429) or a plain ApiException based on http_status and throws it. It
// centralizes the mapping used by the transport layer so callers never
// construct these exceptions by hand.
[[noreturn]] void throw_api_exception(int http_status, int code, std::string message);

}  // namespace detail

}  // namespace novada
