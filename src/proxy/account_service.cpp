#include "novada/proxy/account_service.hpp"

#include "novada/detail/envelope.hpp"
#include "novada/detail/form_builder.hpp"
#include "novada/detail/validator.hpp"

namespace novada::proxy {

void AccountService::create(const CreateAccountParams& params) const {
  detail::Validator v("Account.create");
  v.require_nonzero("product", static_cast<int>(params.product));
  v.require_str("account", params.account);
  v.require_str("password", params.password);
  v.require_nonzero("status", params.status);
  v.check();

  detail::FormBuilder form;
  form.req_int("product", static_cast<int>(params.product));
  form.req_str("account", params.account);
  form.req_str("password", params.password);
  form.req_int("status", params.status);
  form.opt_str("remark", params.remark);
  form.opt_str("limit_flow", params.limit_flow);

  transport_->do_multipart(transport_->base_url(), "/v1/proxy_account/create", form.build());
}

AccountList AccountService::list(const ListAccountParams& params) const {
  detail::Validator v("Account.list");
  v.require_nonzero("product", static_cast<int>(params.product));
  v.check();

  detail::ResolvedPage page = detail::apply_default_page(params.page, params.limit);

  detail::FormBuilder form;
  form.req_int("product", static_cast<int>(params.product));
  form.opt_int("status", params.status);
  form.opt_str("account", params.account);
  form.req_int("page", page.page);
  form.req_int("limit", page.limit);

  nlohmann::json data =
      transport_->do_multipart(transport_->base_url(), "/v1/proxy_account/list", form.build());

  AccountList out;
  detail::unwrap_list(data, out.list, out.total);
  out.page = data.value("page", 0);
  return out;
}

void AccountService::update(const UpdateAccountParams& params) const {
  detail::Validator v("Account.update");
  v.require_nonzero("id", params.id);
  v.require_str("account", params.account);
  v.require_str("password", params.password);
  v.check();

  detail::FormBuilder form;
  form.req_int("id", params.id);
  form.req_str("account", params.account);
  form.req_str("password", params.password);
  form.opt_int("status", params.status);
  form.opt_str("remark", params.remark);
  form.opt_str("limit_flow", params.limit_flow);

  transport_->do_multipart(transport_->base_url(), "/v1/proxy_account/update", form.build());
}

AccountConsumeLogList AccountService::consume_log(const ConsumeLogParams& params) const {
  detail::Validator v("Account.consume_log");
  v.require_nonzero("account_id", params.account_id);
  v.check();

  detail::ResolvedPage page = detail::apply_default_page(params.page, params.limit);

  detail::FormBuilder form;
  form.req_int("account_id", params.account_id);
  form.opt_str("start_time", params.start_time);
  form.opt_str("end_time", params.end_time);
  form.req_int("page", page.page);
  form.req_int("limit", page.limit);

  nlohmann::json data = transport_->do_multipart(transport_->base_url(),
                                                 "/v1/proxy_account/consume_log", form.build());

  AccountConsumeLogList out;
  int ignored_total = 0;
  detail::unwrap_list(data, out.list, ignored_total);
  return out;
}

}  // namespace novada::proxy
