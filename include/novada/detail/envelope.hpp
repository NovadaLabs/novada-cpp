#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace novada::detail {

// Envelope is the uniform wrapper returned by every /v1 management endpoint:
// {"code":0,"data":{...},"msg":"success","timestamp":1732084616}. A code of 0
// indicates success; any other value indicates a business-level failure.
struct Envelope {
  int code = 0;
  std::string msg;
  nlohmann::json data;
  long long timestamp = 0;
};

// parse_envelope parses body into an Envelope. Throws ApiException (with
// code() == 0) when body is not valid JSON.
Envelope parse_envelope(int http_status, const std::string& body);

// decode_envelope parses a 2xx management response body, enforces the
// business code, and returns the inner "data" field. Throws
// AuthException/RateLimitException/ApiException (selected by http_status)
// when the envelope carries a non-zero code, so callers must never treat
// HTTP 200 as success on its own.
nlohmann::json decode_envelope(int http_status, const std::string& body);

// unwrap_list decodes a list-shaped "data" payload of the form
// {"list":[...],"total":N} into out_list/out_total. Requires an ADL-visible
// `from_json(const nlohmann::json&, T&)` for T. Extra fields (e.g. "page")
// are ignored; callers that need them should read data directly.
template <typename T>
void unwrap_list(const nlohmann::json& data, std::vector<T>& out_list, int& out_total) {
  out_list.clear();
  if (data.contains("list") && data.at("list").is_array()) {
    out_list.reserve(data.at("list").size());
    for (const auto& item : data.at("list")) {
      T value{};
      from_json(item, value);
      out_list.push_back(std::move(value));
    }
  }
  out_total = data.value("total", 0);
}

}  // namespace novada::detail
