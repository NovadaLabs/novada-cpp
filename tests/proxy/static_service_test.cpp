#include <string>

#include <gtest/gtest.h>

#include "novada/errors.hpp"
#include "novada/proxy/dedicated_dc_service.hpp"
#include "novada/proxy/static_isp_service.hpp"
#include "support/mock_server.hpp"
#include "support/service_test_util.hpp"

namespace {

using novada::ValidationException;
using novada::proxy::DedicatedDCService;
using novada::proxy::ExportStaticParams;
using novada::proxy::ListStaticParams;
using novada::proxy::OpenDedicatedDCParams;
using novada::proxy::OpenStaticISPParams;
using novada::proxy::RenewSettingParams;
using novada::proxy::RenewStaticParams;
using novada::proxy::StaticISPService;
using novada::test::make_test_transport;
using novada::test::MockServer;

TEST(StaticISPServiceTest, OpenSendsAllRequiredFields) {
  MockServer server;
  std::string ip_type;
  std::string region;
  std::string duration;
  std::string num;
  server.server().Post(
      "/v1/static_house/open", [&](const httplib::Request& req, httplib::Response& res) {
        ip_type = req.get_file_value("ip_type").content;
        region = req.get_file_value("region").content;
        duration = req.get_file_value("duration").content;
        num = req.get_file_value("num").content;
        res.set_content(R"({"code":0,"msg":"ok","data":null})", "application/json");
      });

  auto transport = make_test_transport(server);
  StaticISPService service(transport);
  service.open(OpenStaticISPParams{"premium", "hk:1|us-va:2", "month", 3});

  EXPECT_EQ(ip_type, "premium");
  EXPECT_EQ(region, "hk:1|us-va:2");
  EXPECT_EQ(duration, "month");
  EXPECT_EQ(num, "3");
}

TEST(StaticISPServiceTest, OpenMissingFieldsThrowsValidationException) {
  MockServer server;
  auto transport = make_test_transport(server);
  StaticISPService service(transport);
  try {
    service.open(OpenStaticISPParams{"", "", "", 0});
    FAIL() << "expected ValidationException";
  } catch (const ValidationException& e) {
    ASSERT_EQ(e.fields().size(), 4u);
    EXPECT_EQ(e.fields()[0], "ip_type");
    EXPECT_EQ(e.fields()[3], "num");
  }
}

TEST(StaticISPServiceTest, ListAppliesDefaultsAndOmitsAutoRenewWhenUnset) {
  MockServer server;
  std::string page;
  std::string limit;
  bool auto_renew_seen = false;
  server.server().Post(
      "/v1/static_house/list", [&](const httplib::Request& req, httplib::Response& res) {
        page = req.get_file_value("page").content;
        limit = req.get_file_value("limit").content;
        auto_renew_seen = req.has_file("is_auto_renew");
        res.set_content(
            R"({"code":0,"msg":"ok","data":{"list":[{"id":1,"ip":"1.1.1.1","port":"8080"}],)"
            R"("page":1,"total":1}})",
            "application/json");
      });

  auto transport = make_test_transport(server);
  StaticISPService service(transport);
  auto out = service.list(ListStaticParams{});

  EXPECT_EQ(page, "1");
  EXPECT_EQ(limit, "10");
  EXPECT_FALSE(auto_renew_seen);
  ASSERT_EQ(out.list.size(), 1u);
  EXPECT_EQ(out.list[0].ip, "1.1.1.1");
  EXPECT_EQ(out.list[0].port, "8080");
  EXPECT_EQ(out.total, 1);
  EXPECT_EQ(out.page, 1);
}

TEST(StaticISPServiceTest, ListSendsAutoRenewFilterWhenEngaged) {
  MockServer server;
  std::string auto_renew;
  server.server().Post("/v1/static_house/list", [&](const httplib::Request& req,
                                                    httplib::Response& res) {
    auto_renew = req.get_file_value("is_auto_renew").content;
    res.set_content(R"({"code":0,"msg":"ok","data":{"list":[],"total":0}})", "application/json");
  });

  auto transport = make_test_transport(server);
  StaticISPService service(transport);
  ListStaticParams params;
  params.is_auto_renew = -1;
  (void)service.list(params);

  EXPECT_EQ(auto_renew, "-1");
}

TEST(StaticISPServiceTest, ExportReturnsRawBytesWithoutEnvelope) {
  MockServer server;
  const std::string csv = "id,ip\n1,1.1.1.1\n";
  server.server().Post(
      "/v1/static_house/export",
      [&](const httplib::Request&, httplib::Response& res) { res.set_content(csv, "text/csv"); });

  auto transport = make_test_transport(server);
  StaticISPService service(transport);
  ExportStaticParams params;
  params.status = "1";
  std::string body = service.export_list(params);
  EXPECT_EQ(body, csv);
}

TEST(StaticISPServiceTest, RegionSendsIspTypeAndDecodesGroupedRegions) {
  MockServer server;
  std::string isp_type;
  server.server().Post("/v1/static_house/region", [&](const httplib::Request& req,
                                                      httplib::Response& res) {
    isp_type = req.get_file_value("isp_type").content;
    res.set_content(
        R"({"code":0,"msg":"ok","data":{"list":{"Asia-Pacific":[{"node":"HK","node_en":"Hong Kong",)"
        R"("param":1,"region":"hk"}]}}})",
        "application/json");
  });

  auto transport = make_test_transport(server);
  StaticISPService service(transport);
  auto out = service.region("isp-resi");

  EXPECT_EQ(isp_type, "isp-resi");
  ASSERT_EQ(out.list.count("Asia-Pacific"), 1u);
  ASSERT_EQ(out.list.at("Asia-Pacific").size(), 1u);
  EXPECT_EQ(out.list.at("Asia-Pacific")[0].node_en, "Hong Kong");
  EXPECT_EQ(out.list.at("Asia-Pacific")[0].region, "hk");
}

TEST(StaticISPServiceTest, RegionMissingIspTypeThrowsValidationException) {
  MockServer server;
  auto transport = make_test_transport(server);
  StaticISPService service(transport);
  EXPECT_THROW({ (void)service.region(""); }, ValidationException);
}

TEST(StaticISPServiceTest, RenewSendsIpListAndDuration) {
  MockServer server;
  std::string ip_list;
  std::string duration;
  server.server().Post(
      "/v1/static_house/renew", [&](const httplib::Request& req, httplib::Response& res) {
        ip_list = req.get_file_value("renew_ip_list").content;
        duration = req.get_file_value("duration").content;
        res.set_content(R"({"code":0,"msg":"ok","data":null})", "application/json");
      });

  auto transport = make_test_transport(server);
  StaticISPService service(transport);
  service.renew(RenewStaticParams{"1.1.1.1,2.2.2.2", "week"});

  EXPECT_EQ(ip_list, "1.1.1.1,2.2.2.2");
  EXPECT_EQ(duration, "week");
}

TEST(StaticISPServiceTest, RenewSettingSendsProductTypeStaticHouse) {
  MockServer server;
  std::string type_value;
  std::string status_value;
  std::string renew_type_value;
  server.server().Post(
      "/v1/static_house/renew_setting", [&](const httplib::Request& req, httplib::Response& res) {
        type_value = req.get_file_value("type").content;
        status_value = req.get_file_value("status").content;
        renew_type_value = req.get_file_value("renew_type").content;
        res.set_content(R"({"code":0,"msg":"ok","data":null})", "application/json");
      });

  auto transport = make_test_transport(server);
  StaticISPService service(transport);
  service.renew_setting(RenewSettingParams{"100,111", "month", 1, 2});

  EXPECT_EQ(type_value, "static_house");
  EXPECT_EQ(status_value, "1");
  EXPECT_EQ(renew_type_value, "2");
}

TEST(DedicatedDCServiceTest, OpenUsesStaticPathAndOmitsIpType) {
  MockServer server;
  bool ip_type_seen = false;
  std::string region;
  server.server().Post("/v1/static/open", [&](const httplib::Request& req, httplib::Response& res) {
    ip_type_seen = req.has_file("ip_type");
    region = req.get_file_value("region").content;
    res.set_content(R"({"code":0,"msg":"ok","data":null})", "application/json");
  });

  auto transport = make_test_transport(server);
  DedicatedDCService service(transport);
  service.open(OpenDedicatedDCParams{"hk:1|tw:2", "month", 2});

  EXPECT_FALSE(ip_type_seen);
  EXPECT_EQ(region, "hk:1|tw:2");
}

TEST(DedicatedDCServiceTest, RegionUsesStaticRegionPathWithNoFields) {
  MockServer server;
  server.on_post_json("/v1/static/region", R"({"code":0,"msg":"ok","data":{"list":{}}})");

  auto transport = make_test_transport(server);
  DedicatedDCService service(transport);
  auto out = service.region();
  EXPECT_TRUE(out.list.empty());
}

TEST(DedicatedDCServiceTest, RenewSettingSendsProductTypeStatic) {
  MockServer server;
  std::string type_value;
  server.server().Post(
      "/v1/static/renew_setting", [&](const httplib::Request& req, httplib::Response& res) {
        type_value = req.get_file_value("type").content;
        res.set_content(R"({"code":0,"msg":"ok","data":null})", "application/json");
      });

  auto transport = make_test_transport(server);
  DedicatedDCService service(transport);
  service.renew_setting(RenewSettingParams{"100", "week", 1, 1});
  EXPECT_EQ(type_value, "static");
}

}  // namespace
