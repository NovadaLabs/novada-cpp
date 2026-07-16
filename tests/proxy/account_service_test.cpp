#include "novada/proxy/account_service.hpp"

#include <chrono>
#include <string>

#include <gtest/gtest.h>

#include "novada/errors.hpp"
#include "support/mock_server.hpp"

namespace {

using novada::ValidationException;
using novada::detail::Transport;
using novada::proxy::AccountService;
using novada::proxy::ConsumeLogParams;
using novada::proxy::CreateAccountParams;
using novada::proxy::ListAccountParams;
using novada::proxy::Product;
using novada::proxy::UpdateAccountParams;
using novada::test::MockServer;

constexpr std::chrono::milliseconds kTestTimeout{2000};

Transport make_transport(const MockServer& server) {
  return {"test-key", server.base_url(),    server.base_url(), server.base_url(), kTestTimeout,
          2,          "novada-cpp-test/0.0"};
}

TEST(AccountServiceTest, CreateSendsRequiredAndOptionalFields) {
  MockServer server;
  std::string product_value;
  std::string account_value;
  std::string password_value;
  std::string status_value;
  bool limit_flow_seen = false;

  server.server().Post(
      "/v1/proxy_account/create", [&](const httplib::Request& req, httplib::Response& res) {
        product_value = req.get_file_value("product").content;
        account_value = req.get_file_value("account").content;
        password_value = req.get_file_value("password").content;
        status_value = req.get_file_value("status").content;
        limit_flow_seen = req.has_file("limit_flow");
        res.set_content(R"({"code":0,"msg":"ok","data":null})", "application/json");
      });

  Transport transport = make_transport(server);
  AccountService service(transport);
  CreateAccountParams params;
  params.product = Product::kBrowserAPI;
  params.account = "acct1";
  params.password = "pass1";
  params.status = 1;
  service.create(params);

  EXPECT_EQ(product_value, "10");
  EXPECT_EQ(account_value, "acct1");
  EXPECT_EQ(password_value, "pass1");
  EXPECT_EQ(status_value, "1");
  EXPECT_FALSE(limit_flow_seen);
}

TEST(AccountServiceTest, CreateMissingFieldsReportsAll) {
  MockServer server;
  Transport transport = make_transport(server);
  AccountService service(transport);

  try {
    service.create(CreateAccountParams{});
    FAIL() << "expected ValidationException";
  } catch (const ValidationException& e) {
    EXPECT_EQ(e.method(), "Account.create");
    ASSERT_EQ(e.fields().size(), 4u);
    EXPECT_EQ(e.fields()[0], "product");
    EXPECT_EQ(e.fields()[1], "account");
    EXPECT_EQ(e.fields()[2], "password");
    EXPECT_EQ(e.fields()[3], "status");
  }
}

TEST(AccountServiceTest, ListAppliesDefaultPagination) {
  MockServer server;
  std::string page_value;
  std::string limit_value;
  server.server().Post(
      "/v1/proxy_account/list", [&](const httplib::Request& req, httplib::Response& res) {
        page_value = req.get_file_value("page").content;
        limit_value = req.get_file_value("limit").content;
        res.set_content(R"({"code":0,"msg":"ok","data":{"list":[],"page":1,"total":0}})",
                        "application/json");
      });

  Transport transport = make_transport(server);
  AccountService service(transport);
  ListAccountParams params;
  params.product = Product::kResidential;
  auto out = service.list(params);

  EXPECT_EQ(page_value, "1");
  EXPECT_EQ(limit_value, "10");
  EXPECT_EQ(out.total, 0);
  EXPECT_EQ(out.page, 1);
}

TEST(AccountServiceTest, ListDecodesAccountFields) {
  MockServer server;
  server.server().Post(
      "/v1/proxy_account/list", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"code":0,"msg":"ok","data":{"list":[{"created_at":1731311110,)"
                        R"("updated_at":1732066800,"id":2,"uid":81,"account":"account11",)"
                        R"("password":"pass***","status":1,"residential_balance":0,)"
                        R"("residential_all_buy":0,"residential_status":1,"dc_balance":0,)"
                        R"("dc_all_buy":0,"dc_status":-1,"isp_balance":0,"isp_all_buy":0,)"
                        R"("isp_status":-1,"flow_type":"custom,static,isp,duration",)"
                        R"("account_type":"limit","check_white_list":1,"remark":" ",)"
                        R"("consumed_residential_flow":90665,"limit_residential_flow":1100000000,)"
                        R"("consumed_dc_flow":1997,"limit_dc_flow":0,"consumed_isp_flow":2543,)"
                        R"("limit_isp_flow":0}],"page":1,"total":2}})",
                        "application/json");
      });

  Transport transport = make_transport(server);
  AccountService service(transport);
  ListAccountParams params;
  params.product = Product::kResidential;
  auto out = service.list(params);

  ASSERT_EQ(out.list.size(), 1u);
  EXPECT_EQ(out.list[0].id, 2);
  EXPECT_EQ(out.list[0].account, "account11");
  EXPECT_EQ(out.list[0].flow_type, "custom,static,isp,duration");
  EXPECT_EQ(out.list[0].consumed_residential_flow, 90665);
  EXPECT_EQ(out.total, 2);
  EXPECT_EQ(out.page, 1);
}

TEST(AccountServiceTest, ListMissingProductThrowsValidationException) {
  MockServer server;
  Transport transport = make_transport(server);
  AccountService service(transport);

  EXPECT_THROW({ (void)service.list(ListAccountParams{}); }, ValidationException);
}

TEST(AccountServiceTest, UpdateSendsRequiredFields) {
  MockServer server;
  std::string id_value;
  std::string account_value;
  std::string password_value;
  server.server().Post(
      "/v1/proxy_account/update", [&](const httplib::Request& req, httplib::Response& res) {
        id_value = req.get_file_value("id").content;
        account_value = req.get_file_value("account").content;
        password_value = req.get_file_value("password").content;
        res.set_content(R"({"code":0,"msg":"ok","data":null})", "application/json");
      });

  Transport transport = make_transport(server);
  AccountService service(transport);
  UpdateAccountParams params;
  params.id = 5;
  params.account = "acct5";
  params.password = "pass5";
  service.update(params);

  EXPECT_EQ(id_value, "5");
  EXPECT_EQ(account_value, "acct5");
  EXPECT_EQ(password_value, "pass5");
}

TEST(AccountServiceTest, UpdateMissingFieldsThrowsValidationException) {
  MockServer server;
  Transport transport = make_transport(server);
  AccountService service(transport);

  try {
    service.update(UpdateAccountParams{});
    FAIL() << "expected ValidationException";
  } catch (const ValidationException& e) {
    EXPECT_EQ(e.method(), "Account.update");
    ASSERT_EQ(e.fields().size(), 3u);
    EXPECT_EQ(e.fields()[0], "id");
    EXPECT_EQ(e.fields()[1], "account");
    EXPECT_EQ(e.fields()[2], "password");
  }
}

TEST(AccountServiceTest, ConsumeLogAppliesDefaultPaginationAndDecodesList) {
  MockServer server;
  std::string page_value;
  std::string limit_value;
  server.server().Post(
      "/v1/proxy_account/consume_log", [&](const httplib::Request& req, httplib::Response& res) {
        page_value = req.get_file_value("page").content;
        limit_value = req.get_file_value("limit").content;
        res.set_content(R"({"code":0,"msg":"ok","data":{"list":[{"id":1,"account_id":5,"uid":81,)"
                        R"("residential_balance":1,"residential_all_buy":2,)"
                        R"("consumed_residential_flow":3,"dc_balance":4,"dc_all_buy":5,)"
                        R"("consumed_dc_flow":6,"isp_balance":7,"isp_all_buy":8,)"
                        R"("consumed_isp_flow":9,"day":1732000000}]}})",
                        "application/json");
      });

  Transport transport = make_transport(server);
  AccountService service(transport);
  ConsumeLogParams params;
  params.account_id = 5;
  auto out = service.consume_log(params);

  EXPECT_EQ(page_value, "1");
  EXPECT_EQ(limit_value, "10");
  ASSERT_EQ(out.list.size(), 1u);
  EXPECT_EQ(out.list[0].account_id, 5);
  EXPECT_EQ(out.list[0].day, 1732000000);
}

TEST(AccountServiceTest, ConsumeLogMissingAccountIdThrowsValidationException) {
  MockServer server;
  Transport transport = make_transport(server);
  AccountService service(transport);

  EXPECT_THROW({ (void)service.consume_log(ConsumeLogParams{}); }, ValidationException);
}

}  // namespace
