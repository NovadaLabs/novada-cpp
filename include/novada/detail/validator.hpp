#pragma once

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <vector>

#include "novada/errors.hpp"

namespace novada::detail {

// Validator accumulates the names of missing required fields and, at the end
// of a method's argument check, throws a ValidationException listing all of
// them at once (before any request is sent). It mirrors the Go SDK's
// `validator` helper.
class Validator {
 public:
  explicit Validator(std::string method) : method_(std::move(method)) {}

  // require_str records name as missing when value is empty or whitespace.
  Validator& require_str(const std::string& name, const std::string& value) {
    if (is_blank(value)) {
      missing_.push_back(name);
    }
    return *this;
  }

  // require_nonzero records name as missing when value is zero (used for
  // required integer/enum fields, matching Go's nonZero check).
  Validator& require_nonzero(const std::string& name, long long value) {
    if (value == 0) {
      missing_.push_back(name);
    }
    return *this;
  }

  // check throws a ValidationException when any required field was missing.
  void check() const {
    if (!missing_.empty()) {
      throw ValidationException(method_, missing_);
    }
  }

 private:
  static bool is_blank(const std::string& value) {
    return std::all_of(value.begin(), value.end(),
                       [](unsigned char c) { return std::isspace(c) != 0; });
  }

  std::string method_;
  std::vector<std::string> missing_;
};

// apply_default_page applies the API's pagination defaults (page=1, limit=10)
// when the caller leaves them unset, so list calls work without boilerplate.
// It returns the resolved values directly (rather than mutating page/limit
// through the optionals) so callers never touch std::optional::operator* on a
// value that might still be empty.
struct ResolvedPage {
  int page = 1;
  int limit = 10;
};

inline ResolvedPage apply_default_page(const std::optional<int>& page,
                                       const std::optional<int>& limit) {
  ResolvedPage resolved;
  resolved.page = (page.has_value() && *page != 0) ? *page : 1;
  resolved.limit = (limit.has_value() && *limit != 0) ? *limit : 10;
  return resolved;
}

}  // namespace novada::detail
