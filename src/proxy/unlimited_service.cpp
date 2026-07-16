#include "novada/proxy/unlimited_service.hpp"

#include "novada/detail/envelope.hpp"
#include "novada/detail/form_builder.hpp"
#include "novada/detail/validator.hpp"

namespace novada::proxy {

UnlimitedHostList UnlimitedService::hosts(std::optional<int> page, std::optional<int> limit) const {
  detail::ResolvedPage resolved = detail::apply_default_page(page, limit);

  detail::FormBuilder form;
  form.req_int("page", resolved.page);
  form.req_int("limit", resolved.limit);

  nlohmann::json data =
      transport_->do_multipart(transport_->base_url(), "/v1/unlimited/host_list", form.build());

  UnlimitedHostList out;
  detail::unwrap_list(data, out.list, out.total);
  out.page = data.value("page", 0);
  return out;
}

}  // namespace novada::proxy
