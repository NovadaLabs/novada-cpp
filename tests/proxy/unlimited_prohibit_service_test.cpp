#include <string>

#include <gtest/gtest.h>

#include "novada/errors.hpp"
#include "novada/proxy/prohibit_domain_service.hpp"
#include "novada/proxy/unlimited_service.hpp"
#include "support/mock_server.hpp"
#include "support/service_test_util.hpp"

namespace {

using novada::ValidationException;
using novada::proxy::DeleteProhibitParams;
using novada::proxy::ProhibitDomainService;
using novada::proxy::UnlimitedService;
using novada::test::make_test_transport;
using novada::test::MockServer;

TEST(UnlimitedServiceTest, HostsAppliesDefaultPaginationAndDecodes) {
  MockServer server;
  std::string page;
  std::string limit;
  server.server().Post(
      "/v1/unlimited/host_list", [&](const httplib::Request& req, httplib::Response& res) {
        page = req.get_file_value("page").content;
        limit = req.get_file_value("limit").content;
        res.set_content(R"({"code":0,"msg":"ok","data":{"list":[{"id":1,"host":"h1.example.com",)"
                        R"("state":1,"band_width":1000,"order_id":"o1"}],"page":1,"total":1}})",
                        "application/json");
      });

  auto transport = make_test_transport(server);
  UnlimitedService service(transport);
  auto out = service.hosts();

  EXPECT_EQ(page, "1");
  EXPECT_EQ(limit, "10");
  ASSERT_EQ(out.list.size(), 1u);
  EXPECT_EQ(out.list[0].host, "h1.example.com");
  EXPECT_EQ(out.list[0].band_width, 1000);
  EXPECT_EQ(out.list[0].order_id, "o1");
  EXPECT_EQ(out.total, 1);
  EXPECT_EQ(out.page, 1);
}

TEST(UnlimitedServiceTest, HostsHonorsExplicitPagination) {
  MockServer server;
  std::string page;
  std::string limit;
  server.server().Post("/v1/unlimited/host_list", [&](const httplib::Request& req,
                                                      httplib::Response& res) {
    page = req.get_file_value("page").content;
    limit = req.get_file_value("limit").content;
    res.set_content(R"({"code":0,"msg":"ok","data":{"list":[],"total":0}})", "application/json");
  });

  auto transport = make_test_transport(server);
  UnlimitedService service(transport);
  (void)service.hosts(3, 25);

  EXPECT_EQ(page, "3");
  EXPECT_EQ(limit, "25");
}

TEST(ProhibitDomainServiceTest, AddSendsAddress) {
  MockServer server;
  std::string address;
  server.server().Post(
      "/v1/prohibit_domain/add", [&](const httplib::Request& req, httplib::Response& res) {
        address = req.get_file_value("address").content;
        res.set_content(R"({"code":0,"msg":"ok","data":null})", "application/json");
      });

  auto transport = make_test_transport(server);
  ProhibitDomainService service(transport);
  service.add("example.com");
  EXPECT_EQ(address, "example.com");
}

TEST(ProhibitDomainServiceTest, AddMissingAddressThrowsValidationException) {
  MockServer server;
  auto transport = make_test_transport(server);
  ProhibitDomainService service(transport);
  EXPECT_THROW({ service.add(""); }, ValidationException);
}

TEST(ProhibitDomainServiceTest, ListDecodesEntries) {
  MockServer server;
  server.on_post_json("/v1/prohibit_domain/list",
                      R"({"code":0,"msg":"ok","data":{"list":[{"id":1,"uid":2,"status":1,)"
                      R"("address":"blocked.com","created_at":100,"updated_at":200}],"total":1}})");

  auto transport = make_test_transport(server);
  ProhibitDomainService service(transport);
  auto out = service.list();

  ASSERT_EQ(out.list.size(), 1u);
  EXPECT_EQ(out.list[0].address, "blocked.com");
  EXPECT_EQ(out.list[0].id, 1);
  EXPECT_EQ(out.total, 1);
}

TEST(ProhibitDomainServiceTest, DeleteByIdSendsIsAllTwoAndId) {
  MockServer server;
  std::string is_all;
  std::string id;
  server.server().Post(
      "/v1/prohibit_domain/del", [&](const httplib::Request& req, httplib::Response& res) {
        is_all = req.get_file_value("is_all").content;
        id = req.get_file_value("id").content;
        res.set_content(R"({"code":0,"msg":"ok","data":null})", "application/json");
      });

  auto transport = make_test_transport(server);
  ProhibitDomainService service(transport);
  DeleteProhibitParams params;
  params.id = "42";
  params.all = false;
  service.del(params);

  EXPECT_EQ(is_all, "2");
  EXPECT_EQ(id, "42");
}

TEST(ProhibitDomainServiceTest, DeleteAllSendsIsAllOneAndOmitsId) {
  MockServer server;
  std::string is_all;
  bool id_seen = false;
  server.server().Post(
      "/v1/prohibit_domain/del", [&](const httplib::Request& req, httplib::Response& res) {
        is_all = req.get_file_value("is_all").content;
        id_seen = req.has_file("id");
        res.set_content(R"({"code":0,"msg":"ok","data":null})", "application/json");
      });

  auto transport = make_test_transport(server);
  ProhibitDomainService service(transport);
  DeleteProhibitParams params;
  params.all = true;
  service.del(params);

  EXPECT_EQ(is_all, "1");
  EXPECT_FALSE(id_seen) << "id must be omitted when deleting all entries";
}

TEST(ProhibitDomainServiceTest, DeleteByIdMissingIdThrowsValidationException) {
  MockServer server;
  auto transport = make_test_transport(server);
  ProhibitDomainService service(transport);
  DeleteProhibitParams params;
  params.all = false;
  EXPECT_THROW({ service.del(params); }, ValidationException);
}

}  // namespace
