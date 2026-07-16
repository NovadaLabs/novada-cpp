#pragma once

#include "novada/detail/transport.hpp"
#include "novada/proxy/types.hpp"

namespace novada::proxy {

// RotatingISPService covers rotating ISP proxy areas and traffic (the
// /v1/proxy/isp_data_area and /v1/isp_flow/* endpoints).
class RotatingISPService {
 public:
  explicit RotatingISPService(const detail::Transport& transport) : transport_(&transport) {}

  // countries lists rotating ISP countries (POST /v1/proxy/isp_data_area).
  [[nodiscard]] ISPArea countries() const;

  // balance returns the remaining rotating ISP traffic
  // (POST /v1/isp_flow/balance).
  [[nodiscard]] FlowBalance balance() const;

  // consume_log returns the main account's rotating ISP traffic consumption
  // over a time range (POST /v1/isp_flow/consume_log). Both bounds are
  // required.
  [[nodiscard]] FlowConsumeLogList consume_log(const TimeRange& range) const;

 private:
  const detail::Transport* transport_;
};

// RotatingDCService covers rotating datacenter proxy areas and traffic (the
// /v1/proxy/dynamic_data_area and /v1/dc_flow/* endpoints).
class RotatingDCService {
 public:
  explicit RotatingDCService(const detail::Transport& transport) : transport_(&transport) {}

  // countries lists rotating datacenter countries
  // (POST /v1/proxy/dynamic_data_area).
  [[nodiscard]] DCArea countries() const;

  // balance returns the remaining rotating datacenter traffic
  // (POST /v1/dc_flow/balance).
  [[nodiscard]] FlowBalance balance() const;

  // consume_log returns the main account's rotating datacenter traffic
  // consumption over a time range (POST /v1/dc_flow/consume_log). Both bounds
  // are required.
  [[nodiscard]] FlowConsumeLogList consume_log(const TimeRange& range) const;

 private:
  const detail::Transport* transport_;
};

}  // namespace novada::proxy
