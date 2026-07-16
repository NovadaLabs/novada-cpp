#pragma once

#include <map>
#include <string>

#include "novada/detail/envelope.hpp"
#include "novada/detail/form_builder.hpp"
#include "novada/detail/transport.hpp"
#include "novada/detail/validator.hpp"
#include "novada/proxy/types.hpp"

namespace novada::proxy::internal {

// build_static_filter assembles the shared filter fields used by both the
// list and export static-IP endpoints (status/region/key_word/is_auto_renew).
// Page fields are added by the caller (only list paginates).
inline novada::detail::FormBuilder build_static_filter(const std::string& status,
                                                       const std::string& region,
                                                       const std::string& key_word,
                                                       const std::optional<int>& is_auto_renew) {
  novada::detail::FormBuilder form;
  form.opt_str("status", status);
  form.opt_str("region", region);
  form.opt_str("key_word", key_word);
  form.opt_int("is_auto_renew", is_auto_renew);
  return form;
}

// list_static is the shared implementation for the static ISP and dedicated
// datacenter list endpoints, which share request filters and the StaticIPList
// response shape.
inline StaticIPList list_static(const novada::detail::Transport& transport, const std::string& path,
                                const ListStaticParams& params) {
  novada::detail::ResolvedPage page = novada::detail::apply_default_page(params.page, params.limit);
  novada::detail::FormBuilder form =
      build_static_filter(params.status, params.region, params.key_word, params.is_auto_renew);
  form.req_int("page", page.page);
  form.req_int("limit", page.limit);

  nlohmann::json data = transport.do_multipart(transport.base_url(), path, form.build());

  StaticIPList out;
  novada::detail::unwrap_list(data, out.list, out.total);
  out.page = data.value("page", 0);
  return out;
}

// export_static is the shared implementation for the static ISP and dedicated
// datacenter export endpoints. They return a file stream (typically CSV)
// rather than the JSON envelope, so the raw response body is returned.
inline std::string export_static(const novada::detail::Transport& transport,
                                 const std::string& path, const ExportStaticParams& params) {
  novada::detail::FormBuilder form =
      build_static_filter(params.status, params.region, params.key_word, params.is_auto_renew);
  return transport.do_multipart_raw(transport.base_url(), path, form.build());
}

// renew_static is the shared implementation for the static ISP and dedicated
// datacenter renew endpoints.
inline void renew_static(const novada::detail::Transport& transport, const std::string& method,
                         const std::string& path, const RenewStaticParams& params) {
  novada::detail::Validator v(method);
  v.require_str("renew_ip_list", params.ips);
  v.require_str("duration", params.duration);
  v.check();

  novada::detail::FormBuilder form;
  form.req_str("renew_ip_list", params.ips);
  form.req_str("duration", params.duration);

  transport.do_multipart(transport.base_url(), path, form.build());
}

// renew_setting_static is the shared implementation for the static ISP and
// dedicated datacenter renew_setting endpoints. type_value is the product
// type sent in the "type" field ("static_house" or "static").
inline void renew_setting_static(const novada::detail::Transport& transport,
                                 const std::string& method, const std::string& path,
                                 const std::string& type_value, const RenewSettingParams& params) {
  novada::detail::Validator v(method);
  v.require_str("ids", params.ids);
  v.require_str("package_type", params.package_type);
  v.require_nonzero("status", params.status);
  v.require_nonzero("renew_type", params.renew_type);
  v.check();

  novada::detail::FormBuilder form;
  form.req_str("type", type_value);
  form.req_str("ids", params.ids);
  form.req_str("package_type", params.package_type);
  form.req_int("status", params.status);
  form.req_int("renew_type", params.renew_type);

  transport.do_multipart(transport.base_url(), path, form.build());
}

}  // namespace novada::proxy::internal
