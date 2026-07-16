#pragma once

#include <string>

#include "novada/detail/envelope.hpp"
#include "novada/detail/form_builder.hpp"
#include "novada/detail/transport.hpp"
#include "novada/detail/validator.hpp"
#include "novada/proxy/types.hpp"

namespace novada::proxy::internal {

// consume_log_over_range is the shared implementation for the residential,
// rotating ISP and rotating datacenter traffic-consumption endpoints, which
// share the same request (start_time/end_time, both required) and response
// (a {"list":[...]} of FlowConsumeLog) shapes. It mirrors the Go SDK's
// package-level consumeLog helper.
inline FlowConsumeLogList consume_log_over_range(const novada::detail::Transport& transport,
                                                 const std::string& method, const std::string& path,
                                                 const TimeRange& range) {
  novada::detail::Validator v(method);
  v.require_str("start_time", range.start);
  v.require_str("end_time", range.end);
  v.check();

  novada::detail::FormBuilder form;
  form.req_str("start_time", range.start);
  form.req_str("end_time", range.end);

  nlohmann::json data = transport.do_multipart(transport.base_url(), path, form.build());

  FlowConsumeLogList out;
  int ignored_total = 0;
  novada::detail::unwrap_list(data, out.list, ignored_total);
  return out;
}

}  // namespace novada::proxy::internal
