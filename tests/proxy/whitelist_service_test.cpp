#include "novada/proxy/whitelist_service.hpp"

#include <atomic>
#include <chrono>
#include <string>

#include <gtest/gtest.h>

#include "novada/errors.hpp"
#include "support/mock_server.hpp"

namespace {

using novada::ValidationException;
using novada::detail::Transport;
using novada::proxy::AddWhitelistParams;
using novada::proxy::DeleteWhitelistParams;
using novada::proxy::ListWhitelistParams;
using novada::proxy::Product;
using novada::proxy::RemarkWhitelistParams;
using novada::proxy::WhitelistService;
using novada::test::MockServer;

constexpr std::chrono::milliseconds kTestTimeout{2000};

Transport make_transport(const MockServer& server) {
  return {"test-key", server.base_url(),    server.base_url(), server.base_url(), kTestTimeout,
          2,          "novada-cpp-test/0.0"};
}

TEST(WhitelistServiceTest, AddSendsRequiredAndOptionalFields) {
  MockServer server;
  bool product_seen = false;
  bool ip_seen = false;
  bool remark_seen = false;
  std::string product_value;
  std::string ip_value;
  std::string remark_value;

  server.server().Post(
      "/v1/white_list/add", [&](const httplib::Request& req, httplib::Response& res) {
        product_seen = req.has_file("product");
        ip_seen = req.has_file("ip");
        remark_seen = req.has_file("remark");
        product_value = req.get_file_value("product").content;
        ip_value = req.get_file_value("ip").content;
        remark_value = req.get_file_value("remark").content;
        res.set_content(R"({"code":0,"msg":"success","data":null})", "application/json");
      });

  Transport transport = make_transport(server);
  WhitelistService service(transport);
  EXPECT_NO_THROW(
      { service.add(AddWhitelistParams{Product::kResidential, "1.1.1.1", "my remark"}); });

  EXPECT_TRUE(product_seen);
  EXPECT_EQ(product_value, "1");
  EXPECT_TRUE(ip_seen);
  EXPECT_EQ(ip_value, "1.1.1.1");
  EXPECT_TRUE(remark_seen);
  EXPECT_EQ(remark_value, "my remark");
}

TEST(WhitelistServiceTest, AddOmitsEmptyOptionalRemark) {
  MockServer server;
  bool remark_seen = false;

  server.server().Post(
      "/v1/white_list/add", [&](const httplib::Request& req, httplib::Response& res) {
        remark_seen = req.has_file("remark");
        res.set_content(R"({"code":0,"msg":"success","data":null})", "application/json");
      });

  Transport transport = make_transport(server);
  WhitelistService service(transport);
  service.add(AddWhitelistParams{Product::kResidential, "1.1.1.1", ""});

  EXPECT_FALSE(remark_seen);
}

TEST(WhitelistServiceTest, AddMissingProductThrowsValidationException) {
  MockServer server;
  Transport transport = make_transport(server);
  WhitelistService service(transport);

  try {
    service.add(AddWhitelistParams{static_cast<Product>(0), "1.1.1.1", ""});
    FAIL() << "expected ValidationException";
  } catch (const ValidationException& e) {
    EXPECT_EQ(e.method(), "Whitelist.add");
    ASSERT_EQ(e.fields().size(), 1u);
    EXPECT_EQ(e.fields()[0], "product");
  }
}

TEST(WhitelistServiceTest, AddMissingIpThrowsValidationException) {
  MockServer server;
  Transport transport = make_transport(server);
  WhitelistService service(transport);

  try {
    service.add(AddWhitelistParams{Product::kResidential, "", ""});
    FAIL() << "expected ValidationException";
  } catch (const ValidationException& e) {
    ASSERT_EQ(e.fields().size(), 1u);
    EXPECT_EQ(e.fields()[0], "ip");
  }
}

TEST(WhitelistServiceTest, AddMissingBothReportsBothFields) {
  MockServer server;
  Transport transport = make_transport(server);
  WhitelistService service(transport);

  try {
    service.add(AddWhitelistParams{static_cast<Product>(0), "", ""});
    FAIL() << "expected ValidationException";
  } catch (const ValidationException& e) {
    ASSERT_EQ(e.fields().size(), 2u);
    EXPECT_EQ(e.fields()[0], "product");
    EXPECT_EQ(e.fields()[1], "ip");
  }
}

TEST(WhitelistServiceTest, ListDecodesEntriesAndTotal) {
  MockServer server;
  server.server().Post("/v1/white_list/list", [](const httplib::Request&, httplib::Response& res) {
    res.set_content(R"({"code":0,"msg":"success","data":{"list":[)"
                    R"({"created_at":1732084553,"updated_at":1732084554,)"
                    R"("id":12,"uid":81,"mark_ip":"10.10.10.1","ip":168430081,)"
                    R"("status":1,"lock":false,"lock_uid":0,"flag":0,)"
                    R"("is_biger":0,"is_limit":0,"mark":"test"}],"total":1}})",
                    "application/json");
  });

  Transport transport = make_transport(server);
  WhitelistService service(transport);
  auto out = service.list(ListWhitelistParams{Product::kResidential, "", "", "", std::nullopt});

  EXPECT_EQ(out.total, 1);
  ASSERT_EQ(out.list.size(), 1u);
  EXPECT_EQ(out.list[0].id, 12);
  EXPECT_EQ(out.list[0].mark_ip, "10.10.10.1");
  EXPECT_EQ(out.list[0].ip, 168430081);
  EXPECT_FALSE(out.list[0].lock);
  EXPECT_EQ(out.list[0].mark, "test");
}

TEST(WhitelistServiceTest, ListSendsLockFilterOnlyWhenEngaged) {
  MockServer server;
  bool lock_seen = false;
  std::string lock_value;
  server.server().Post("/v1/white_list/list", [&](const httplib::Request& req,
                                                  httplib::Response& res) {
    lock_seen = req.has_file("lock");
    if (lock_seen) {
      lock_value = req.get_file_value("lock").content;
    }
    res.set_content(R"({"code":0,"msg":"ok","data":{"list":[],"total":0}})", "application/json");
  });

  Transport transport = make_transport(server);
  WhitelistService service(transport);
  (void)service.list(ListWhitelistParams{Product::kResidential, "", "", "", 0});

  EXPECT_TRUE(lock_seen) << "an engaged optional<int> of 0 must still be sent";
  EXPECT_EQ(lock_value, "0");
}

TEST(WhitelistServiceTest, ListMissingProductThrowsValidationException) {
  MockServer server;
  Transport transport = make_transport(server);
  WhitelistService service(transport);

  EXPECT_THROW(
      {
        (void)service.list(ListWhitelistParams{static_cast<Product>(0), "", "", "", std::nullopt});
      },
      ValidationException);
}

TEST(WhitelistServiceTest, DelSendsCommaSeparatedIps) {
  MockServer server;
  std::string ips_value;
  server.server().Post(
      "/v1/white_list/del", [&](const httplib::Request& req, httplib::Response& res) {
        ips_value = req.get_file_value("ips").content;
        res.set_content(R"({"code":0,"msg":"ok","data":null})", "application/json");
      });

  Transport transport = make_transport(server);
  WhitelistService service(transport);
  service.del(DeleteWhitelistParams{Product::kResidential, "1.1.1.1,2.2.2.2"});

  EXPECT_EQ(ips_value, "1.1.1.1,2.2.2.2");
}

TEST(WhitelistServiceTest, DelMissingIpsThrowsValidationException) {
  MockServer server;
  Transport transport = make_transport(server);
  WhitelistService service(transport);

  EXPECT_THROW(
      { service.del(DeleteWhitelistParams{Product::kResidential, ""}); }, ValidationException);
}

TEST(WhitelistServiceTest, RemarkSendsIdAndRemark) {
  MockServer server;
  std::string id_value;
  std::string remark_value;
  server.server().Post(
      "/v1/white_list/remark", [&](const httplib::Request& req, httplib::Response& res) {
        id_value = req.get_file_value("id").content;
        remark_value = req.get_file_value("remark").content;
        res.set_content(R"({"code":0,"msg":"ok","data":null})", "application/json");
      });

  Transport transport = make_transport(server);
  WhitelistService service(transport);
  service.remark(RemarkWhitelistParams{Product::kResidential, "12", "updated"});

  EXPECT_EQ(id_value, "12");
  EXPECT_EQ(remark_value, "updated");
}

TEST(WhitelistServiceTest, RemarkMissingIdThrowsValidationException) {
  MockServer server;
  Transport transport = make_transport(server);
  WhitelistService service(transport);

  EXPECT_THROW(
      { service.remark(RemarkWhitelistParams{Product::kResidential, "", ""}); },
      ValidationException);
}

TEST(WhitelistServiceTest, NonZeroCodePropagatesAsApiException) {
  MockServer server;
  server.server().Post("/v1/white_list/add", [](const httplib::Request&, httplib::Response& res) {
    res.set_content(R"({"code":1001,"msg":"bad ip","data":null})", "application/json");
  });

  Transport transport = make_transport(server);
  WhitelistService service(transport);

  try {
    service.add(AddWhitelistParams{Product::kResidential, "1.1.1.1", ""});
    FAIL() << "expected ApiException";
  } catch (const novada::ApiException& e) {
    EXPECT_EQ(e.code(), 1001);
  }
}

}  // namespace
