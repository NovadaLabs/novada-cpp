#pragma once

#include <map>
#include <optional>
#include <string>

namespace novada::detail {

// FormBuilder assembles the multipart/urlencoded field map for a request. It
// mirrors the Go SDK's `form` helper: required setters always write the field
// (so the server-side required check is satisfied), while optional setters
// drop empty strings and unset std::optional values -- nullopt/"" means "do
// not send", matching Go's zero-value omission semantics. The transport layer
// additionally skips any field whose value ends up empty.
class FormBuilder {
 public:
  // req_str writes a required string field verbatim.
  FormBuilder& req_str(const std::string& key, const std::string& value) {
    fields_[key] = value;
    return *this;
  }

  // opt_str writes value only when it is non-empty.
  FormBuilder& opt_str(const std::string& key, const std::string& value) {
    if (!value.empty()) {
      fields_[key] = value;
    }
    return *this;
  }

  // req_int writes a required integer field.
  FormBuilder& req_int(const std::string& key, long long value) {
    fields_[key] = std::to_string(value);
    return *this;
  }

  // opt_int writes value only when it is non-zero, matching Go's optInt.
  FormBuilder& opt_int(const std::string& key, long long value) {
    if (value != 0) {
      fields_[key] = std::to_string(value);
    }
    return *this;
  }

  // opt_int writes the wrapped integer only when the optional is engaged.
  // Unlike opt_int(long long), an engaged value of 0 IS sent, mirroring Go's
  // optIntPtr(*int) where a non-nil pointer to 0 is transmitted.
  FormBuilder& opt_int(const std::string& key, const std::optional<int>& value) {
    if (value.has_value()) {
      fields_[key] = std::to_string(*value);
    }
    return *this;
  }

  // opt_bool writes "true"/"false" only when the optional is engaged,
  // mirroring Go's `if p.Field != nil { values.Set(key, strconv.FormatBool(*p.Field)) }`.
  FormBuilder& opt_bool(const std::string& key, const std::optional<bool>& value) {
    if (value.has_value()) {
      fields_[key] = *value ? "true" : "false";
    }
    return *this;
  }

  // build returns the assembled field map.
  [[nodiscard]] const std::map<std::string, std::string>& build() const { return fields_; }

 private:
  std::map<std::string, std::string> fields_;
};

}  // namespace novada::detail
