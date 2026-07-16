#include "novada/proxy/prohibit_domain_service.hpp"

#include "novada/detail/envelope.hpp"
#include "novada/detail/form_builder.hpp"
#include "novada/detail/validator.hpp"

namespace novada::proxy {

void ProhibitDomainService::add(const std::string& address) const {
  detail::Validator v("ProhibitDomain.add");
  v.require_str("address", address);
  v.check();

  detail::FormBuilder form;
  form.req_str("address", address);

  transport_->do_multipart(transport_->base_url(), "/v1/prohibit_domain/add", form.build());
}

ProhibitDomainList ProhibitDomainService::list() const {
  nlohmann::json data =
      transport_->do_multipart(transport_->base_url(), "/v1/prohibit_domain/list", {});
  ProhibitDomainList out;
  detail::unwrap_list(data, out.list, out.total);
  return out;
}

void ProhibitDomainService::del(const DeleteProhibitParams& params) const {
  detail::Validator v("ProhibitDomain.del");
  if (!params.all) {
    v.require_str("id", params.id);
  }
  v.check();

  // is_all: "1" deletes every entry, "2" deletes the single id, matching the
  // Go SDK's encoding.
  detail::FormBuilder form;
  form.req_str("is_all", params.all ? "1" : "2");
  form.opt_str("id", params.id);

  transport_->do_multipart(transport_->base_url(), "/v1/prohibit_domain/del", form.build());
}

}  // namespace novada::proxy
