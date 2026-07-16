#include "novada/errors.hpp"

#include <sstream>

namespace novada {

std::string ApiException::format_message(int http_status, int code, const std::string& message) {
  std::ostringstream oss;
  if (code != 0) {
    oss << "novada: api error (code=" << code << ", http=" << http_status << "): " << message;
  } else {
    oss << "novada: http error (status=" << http_status << "): " << message;
  }
  return oss.str();
}

std::string ValidationException::format_message(const std::string& method,
                                                const std::vector<std::string>& fields) {
  std::ostringstream oss;
  oss << "novada: " << method << ": missing required field(s): ";
  for (std::size_t i = 0; i < fields.size(); ++i) {
    if (i != 0) {
      oss << ", ";
    }
    oss << fields[i];
  }
  return oss.str();
}

bool is_auth_error(const ApiException& e) noexcept {
  return e.http_status() == 401 || e.http_status() == 403;
}

bool is_rate_limited(const ApiException& e) noexcept {
  return e.http_status() == 429;
}

namespace detail {

void throw_api_exception(int http_status, int code, std::string message) {
  if (http_status == 401 || http_status == 403) {
    throw AuthException(http_status, code, std::move(message));
  }
  if (http_status == 429) {
    throw RateLimitException(http_status, code, std::move(message));
  }
  throw ApiException(http_status, code, std::move(message));
}

}  // namespace detail

}  // namespace novada
