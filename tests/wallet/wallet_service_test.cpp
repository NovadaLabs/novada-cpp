#include "novada/wallet/wallet_service.hpp"

#include <string>

#include <gtest/gtest.h>

#include "novada/errors.hpp"
#include "support/mock_server.hpp"
#include "support/service_test_util.hpp"

namespace {

using novada::test::make_test_transport;
using novada::test::MockServer;
using novada::wallet::WalletService;

TEST(WalletServiceTest, BalanceDecodesBalance) {
  MockServer server;
  server.on_post_json("/v1/wallet/balance", R"({"code":0,"msg":"ok","data":{"balance":104}})");

  auto transport = make_test_transport(server);
  WalletService service(transport);
  auto out = service.balance();
  EXPECT_EQ(out.balance, 104);
}

TEST(WalletServiceTest, BalancePropagatesApiException) {
  MockServer server;
  server.on_post_json("/v1/wallet/balance", R"({"code":5001,"msg":"wallet locked","data":null})");

  auto transport = make_test_transport(server);
  WalletService service(transport);
  try {
    (void)service.balance();
    FAIL() << "expected ApiException";
  } catch (const novada::ApiException& e) {
    EXPECT_EQ(e.code(), 5001);
    EXPECT_EQ(e.message(), "wallet locked");
  }
}

TEST(WalletServiceTest, UsageRecordAppliesDefaultPaginationAndDecodes) {
  MockServer server;
  std::string page;
  std::string limit;
  server.server().Post(
      "/v1/wallet/usage_record", [&](const httplib::Request& req, httplib::Response& res) {
        page = req.get_file_value("page").content;
        limit = req.get_file_value("limit").content;
        res.set_content(
            R"({"code":0,"msg":"ok","data":{"count":37,"list":[{"created_at":1732106923,)"
            R"("id":845,"uid":81,"order_type":"static_open","type":1,"source":5,)"
            R"("order_id":"2411202048427491","money":1.5,"pay_money":1.5,"pay_type":"wallet",)"
            R"("pay_status":1,"description":"open","status":1,"is_first_order":0,)"
            R"("pay_time":1732106923,"closed_at":0}]}})",
            "application/json");
      });

  auto transport = make_test_transport(server);
  WalletService service(transport);
  auto out = service.usage_record();

  EXPECT_EQ(page, "1");
  EXPECT_EQ(limit, "10");
  EXPECT_EQ(out.count, 37);
  ASSERT_EQ(out.list.size(), 1u);
  EXPECT_EQ(out.list[0].id, 845);
  EXPECT_EQ(out.list[0].order_type, "static_open");
  EXPECT_EQ(out.list[0].order_id, "2411202048427491");
  EXPECT_DOUBLE_EQ(out.list[0].money, 1.5);
  EXPECT_EQ(out.list[0].pay_type, "wallet");
}

TEST(WalletServiceTest, UsageRecordHonorsExplicitPagination) {
  MockServer server;
  std::string page;
  std::string limit;
  server.server().Post("/v1/wallet/usage_record", [&](const httplib::Request& req,
                                                      httplib::Response& res) {
    page = req.get_file_value("page").content;
    limit = req.get_file_value("limit").content;
    res.set_content(R"({"code":0,"msg":"ok","data":{"count":0,"list":[]}})", "application/json");
  });

  auto transport = make_test_transport(server);
  WalletService service(transport);
  auto out = service.usage_record(2, 50);

  EXPECT_EQ(page, "2");
  EXPECT_EQ(limit, "50");
  EXPECT_EQ(out.count, 0);
  EXPECT_TRUE(out.list.empty());
}

}  // namespace
