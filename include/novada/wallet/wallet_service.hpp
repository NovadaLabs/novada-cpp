#pragma once

#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "novada/detail/transport.hpp"

namespace novada::wallet {

// Balance is the wallet balance payload.
struct Balance {
  long long balance = 0;
};
void from_json(const nlohmann::json& j, Balance& v);

// UsageRecord is a single wallet usage/order record.
struct UsageRecord {
  long long created_at = 0;
  long long updated_at = 0;
  int id = 0;
  int uid = 0;
  std::string order_type;
  int type = 0;
  int source = 0;
  std::string order_id;
  double money = 0;
  double pay_money = 0;
  std::string pay_type;
  std::string pay_sn;
  int pay_status = 0;
  int pay_cate = 0;
  std::string description;
  int status = 0;
  std::string create_admin;
  int is_first_order = 0;
  long long pay_time = 0;
  long long closed_at = 0;
};
void from_json(const nlohmann::json& j, UsageRecord& v);

// UsageRecordList is the data payload of WalletService::usage_record.
struct UsageRecordList {
  int count = 0;
  std::vector<UsageRecord> list;
};

// WalletService covers the wallet endpoints (the /v1/wallet/* endpoints).
class WalletService {
 public:
  explicit WalletService(const detail::Transport& transport) : transport_(&transport) {}

  // balance returns the wallet balance (POST /v1/wallet/balance).
  [[nodiscard]] Balance balance() const;

  // usage_record returns paginated wallet usage records
  // (POST /v1/wallet/usage_record). page and limit default to 1 and 10 when
  // left unset.
  [[nodiscard]] UsageRecordList usage_record(std::optional<int> page = std::nullopt,
                                             std::optional<int> limit = std::nullopt) const;

 private:
  const detail::Transport* transport_;
};

}  // namespace novada::wallet
