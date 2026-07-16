#include "novada/proxy/whitelist_service.hpp"

#include "novada/detail/envelope.hpp"
#include "novada/detail/form_builder.hpp"
#include "novada/detail/validator.hpp"

namespace novada::proxy {

void WhitelistService::add(const AddWhitelistParams& params) const {
  detail::Validator v("Whitelist.add");
  v.require_nonzero("product", static_cast<int>(params.product));
  v.require_str("ip", params.ip);
  v.check();

  detail::FormBuilder form;
  form.req_int("product", static_cast<int>(params.product));
  form.req_str("ip", params.ip);
  form.opt_str("remark", params.remark);

  transport_->do_multipart(transport_->base_url(), "/v1/white_list/add", form.build());
}

WhitelistList WhitelistService::list(const ListWhitelistParams& params) const {
  detail::Validator v("Whitelist.list");
  v.require_nonzero("product", static_cast<int>(params.product));
  v.check();

  detail::FormBuilder form;
  form.req_int("product", static_cast<int>(params.product));
  form.opt_str("ip", params.ip);
  form.opt_str("start_time", params.start_time);
  form.opt_str("end_time", params.end_time);
  form.opt_int("lock", params.lock);

  nlohmann::json data =
      transport_->do_multipart(transport_->base_url(), "/v1/white_list/list", form.build());

  WhitelistList out;
  detail::unwrap_list(data, out.list, out.total);
  return out;
}

void WhitelistService::del(const DeleteWhitelistParams& params) const {
  detail::Validator v("Whitelist.del");
  v.require_nonzero("product", static_cast<int>(params.product));
  v.require_str("ips", params.ips);
  v.check();

  detail::FormBuilder form;
  form.req_int("product", static_cast<int>(params.product));
  form.req_str("ips", params.ips);

  transport_->do_multipart(transport_->base_url(), "/v1/white_list/del", form.build());
}

void WhitelistService::remark(const RemarkWhitelistParams& params) const {
  detail::Validator v("Whitelist.remark");
  v.require_nonzero("product", static_cast<int>(params.product));
  v.require_str("id", params.id);
  v.check();

  detail::FormBuilder form;
  form.req_int("product", static_cast<int>(params.product));
  form.req_str("id", params.id);
  form.opt_str("remark", params.remark);

  transport_->do_multipart(transport_->base_url(), "/v1/white_list/remark", form.build());
}

}  // namespace novada::proxy
