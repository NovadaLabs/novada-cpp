#pragma once

#include <string>

#include "novada/detail/transport.hpp"
#include "novada/proxy/types.hpp"

namespace novada::proxy {

// MobileService covers mobile proxy areas and traffic (the
// /v1/proxy/mobile_area and /v1/mobile_flow/* endpoints).
class MobileService {
 public:
  explicit MobileService(const detail::Transport& transport) : transport_(&transport) {}

  // countries lists mobile proxy countries (POST /v1/proxy/mobile_area). The
  // API does not document a fixed response shape, so the raw JSON of the
  // envelope's "data" field is returned as a string for the caller to decode
  // (mirroring the Go SDK returning json.RawMessage).
  [[nodiscard]] std::string countries() const;

  // balance returns the remaining mobile traffic
  // (POST /v1/mobile_flow/mobile_flow_balance).
  [[nodiscard]] MobileBalance balance() const;

  // consume_log returns the main account's mobile traffic consumption
  // (POST /v1/mobile_flow/mobile_flow_use). start, end and granularity are
  // required.
  [[nodiscard]] FlowConsumeLogList consume_log(const MobileUseParams& params) const;

 private:
  const detail::Transport* transport_;
};

}  // namespace novada::proxy
