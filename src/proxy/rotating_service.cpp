#include "novada/proxy/rotating_service.hpp"

#include "novada/detail/envelope.hpp"
#include "proxy/flow_helpers.hpp"

namespace novada::proxy {

// --- Rotating ISP ----------------------------------------------------------

ISPArea RotatingISPService::countries() const {
  nlohmann::json data =
      transport_->do_multipart(transport_->base_url(), "/v1/proxy/isp_data_area", {});
  ISPArea out;
  from_json(data, out);
  return out;
}

FlowBalance RotatingISPService::balance() const {
  nlohmann::json data =
      transport_->do_multipart(transport_->base_url(), "/v1/isp_flow/balance", {});
  FlowBalance out;
  from_json(data, out);
  return out;
}

FlowConsumeLogList RotatingISPService::consume_log(const TimeRange& range) const {
  return internal::consume_log_over_range(*transport_, "RotatingISP.consume_log",
                                          "/v1/isp_flow/consume_log", range);
}

// --- Rotating DC -----------------------------------------------------------

DCArea RotatingDCService::countries() const {
  nlohmann::json data =
      transport_->do_multipart(transport_->base_url(), "/v1/proxy/dynamic_data_area", {});
  DCArea out;
  from_json(data, out);
  return out;
}

FlowBalance RotatingDCService::balance() const {
  nlohmann::json data = transport_->do_multipart(transport_->base_url(), "/v1/dc_flow/balance", {});
  FlowBalance out;
  from_json(data, out);
  return out;
}

FlowConsumeLogList RotatingDCService::consume_log(const TimeRange& range) const {
  return internal::consume_log_over_range(*transport_, "RotatingDC.consume_log",
                                          "/v1/dc_flow/consume_log", range);
}

}  // namespace novada::proxy
