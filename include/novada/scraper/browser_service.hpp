#pragma once

#include <string>

#include "novada/detail/transport.hpp"
#include "novada/scraper/types.hpp"

namespace novada::scraper {

// BrowserService exposes the Browser API area and traffic queries on the
// general host.
class BrowserService {
 public:
  explicit BrowserService(const detail::Transport& transport) : transport_(&transport) {}

  // countries lists Browser API countries (POST /v1/proxy/browser_area). The
  // API does not document a fixed response shape, so the raw JSON of the
  // data payload is returned as a string for the caller to decode (mirroring
  // the Go SDK returning json.RawMessage).
  [[nodiscard]] std::string countries() const;

  // flow_use returns the main account's Browser API traffic consumption over a
  // time range (POST /v1/browser_flow/browser_flow_use). Both bounds use the
  // "2006-01-02 15:04:05" layout and are required.
  [[nodiscard]] BrowserFlowUseList flow_use(const std::string& start, const std::string& end) const;

 private:
  const detail::Transport* transport_;
};

}  // namespace novada::scraper
