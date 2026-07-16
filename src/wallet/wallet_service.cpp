#include "novada/wallet/wallet_service.hpp"

#include "novada/detail/envelope.hpp"
#include "novada/detail/form_builder.hpp"
#include "novada/detail/validator.hpp"

namespace novada::wallet {

void from_json(const nlohmann::json& j, Balance& v) {
  v.balance = j.value("balance", 0LL);
}

void from_json(const nlohmann::json& j, UsageRecord& v) {
  v.created_at = j.value("created_at", 0LL);
  v.updated_at = j.value("updated_at", 0LL);
  v.id = j.value("id", 0);
  v.uid = j.value("uid", 0);
  v.order_type = j.value("order_type", std::string());
  v.type = j.value("type", 0);
  v.source = j.value("source", 0);
  v.order_id = j.value("order_id", std::string());
  v.money = j.value("money", 0.0);
  v.pay_money = j.value("pay_money", 0.0);
  v.pay_type = j.value("pay_type", std::string());
  v.pay_sn = j.value("pay_sn", std::string());
  v.pay_status = j.value("pay_status", 0);
  v.pay_cate = j.value("pay_cate", 0);
  v.description = j.value("description", std::string());
  v.status = j.value("status", 0);
  v.create_admin = j.value("create_admin", std::string());
  v.is_first_order = j.value("is_first_order", 0);
  v.pay_time = j.value("pay_time", 0LL);
  v.closed_at = j.value("closed_at", 0LL);
}

Balance WalletService::balance() const {
  nlohmann::json data = transport_->do_multipart(transport_->base_url(), "/v1/wallet/balance", {});
  Balance out;
  from_json(data, out);
  return out;
}

UsageRecordList WalletService::usage_record(std::optional<int> page,
                                            std::optional<int> limit) const {
  detail::ResolvedPage resolved = detail::apply_default_page(page, limit);

  detail::FormBuilder form;
  form.req_int("page", resolved.page);
  form.req_int("limit", resolved.limit);

  nlohmann::json data =
      transport_->do_multipart(transport_->base_url(), "/v1/wallet/usage_record", form.build());

  UsageRecordList out;
  out.count = data.value("count", 0);
  if (data.contains("list") && data.at("list").is_array()) {
    for (const auto& item : data.at("list")) {
      UsageRecord record;
      from_json(item, record);
      out.list.push_back(std::move(record));
    }
  }
  return out;
}

}  // namespace novada::wallet
