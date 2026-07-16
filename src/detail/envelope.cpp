#include "novada/detail/envelope.hpp"

#include "novada/errors.hpp"

namespace novada::detail {

Envelope parse_envelope(int http_status, const std::string& body) {
  nlohmann::json parsed;
  try {
    parsed = nlohmann::json::parse(body);
  } catch (const nlohmann::json::parse_error& e) {
    throw_api_exception(http_status, 0, std::string("invalid response body: ") + e.what());
  }

  Envelope env;
  env.code = parsed.value("code", 0);
  env.msg = parsed.value("msg", std::string());
  env.timestamp = parsed.value("timestamp", 0LL);
  if (parsed.contains("data")) {
    env.data = parsed.at("data");
  }
  return env;
}

nlohmann::json decode_envelope(int http_status, const std::string& body) {
  Envelope env = parse_envelope(http_status, body);
  if (env.code != 0) {
    throw_api_exception(http_status, env.code, env.msg);
  }
  return env.data;
}

}  // namespace novada::detail
