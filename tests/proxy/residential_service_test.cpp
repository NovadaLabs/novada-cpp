#include "novada/proxy/residential_service.hpp"

#include <string>

#include <gtest/gtest.h>

#include "novada/errors.hpp"
#include "support/mock_server.hpp"
#include "support/service_test_util.hpp"

namespace {

using novada::ValidationException;
using novada::proxy::CityParams;
using novada::proxy::ResidentialService;
using novada::proxy::TimeRange;
using novada::test::make_test_transport;
using novada::test::MockServer;

TEST(ResidentialServiceTest, CountriesDecodesContinentAndCountryGroups) {
  MockServer server;
  server.on_post_json("/v1/proxy/domestic_dynamic_area",
                      R"({"code":0,"msg":"ok","data":{"continent":{"AS":"Asia","EU":"Europe"},)"
                      R"("country":[{"continent":"Asia","continent_code":1,"list":[)"
                      R"({"code":"hk","continent":1,"ip_num":100,"name":"Hong Kong",)"
                      R"("name_en":"Hong Kong","protocol":1}]}]}})");

  auto transport = make_test_transport(server);
  ResidentialService service(transport);
  auto out = service.countries();

  EXPECT_EQ(out.continent.at("AS"), "Asia");
  EXPECT_EQ(out.continent.at("EU"), "Europe");
  ASSERT_EQ(out.country.size(), 1u);
  EXPECT_EQ(out.country[0].continent, "Asia");
  EXPECT_EQ(out.country[0].continent_code, 1);
  ASSERT_EQ(out.country[0].list.size(), 1u);
  EXPECT_EQ(out.country[0].list[0].code, "hk");
  EXPECT_EQ(out.country[0].list[0].name_en, "Hong Kong");
  EXPECT_EQ(out.country[0].list[0].ip_num, 100);
}

TEST(ResidentialServiceTest, StatesSendsCodeAndDecodesDataArray) {
  MockServer server;
  std::string code_value;
  server.server().Post(
      "/v1/proxy/city_by_code", [&](const httplib::Request& req, httplib::Response& res) {
        code_value = req.get_file_value("code").content;
        res.set_content(R"({"code":0,"msg":"ok","data":{"data":[{"state":"alabama"},)"
                        R"({"state":"alaska"}]}})",
                        "application/json");
      });

  auto transport = make_test_transport(server);
  ResidentialService service(transport);
  auto out = service.states("us");

  EXPECT_EQ(code_value, "us");
  ASSERT_EQ(out.data.size(), 2u);
  EXPECT_EQ(out.data[0].state, "alabama");
  EXPECT_EQ(out.data[1].state, "alaska");
}

TEST(ResidentialServiceTest, StatesMissingCodeThrowsValidationException) {
  MockServer server;
  auto transport = make_test_transport(server);
  ResidentialService service(transport);
  EXPECT_THROW({ (void)service.states(""); }, ValidationException);
}

TEST(ResidentialServiceTest, CitiesSendsCodeAndRegion) {
  MockServer server;
  std::string code_value;
  std::string region_value;
  server.server().Post(
      "/v1/proxy/region_by_city", [&](const httplib::Request& req, httplib::Response& res) {
        code_value = req.get_file_value("code").content;
        region_value = req.get_file_value("region").content;
        res.set_content(R"({"code":0,"msg":"ok","data":{"data":[{"code":"montgomery"}]}})",
                        "application/json");
      });

  auto transport = make_test_transport(server);
  ResidentialService service(transport);
  auto out = service.cities(CityParams{"us", "alabama"});

  EXPECT_EQ(code_value, "us");
  EXPECT_EQ(region_value, "alabama");
  ASSERT_EQ(out.data.size(), 1u);
  EXPECT_EQ(out.data[0].code, "montgomery");
}

TEST(ResidentialServiceTest, CitiesMissingRegionThrowsValidationException) {
  MockServer server;
  auto transport = make_test_transport(server);
  ResidentialService service(transport);
  EXPECT_THROW({ (void)service.cities(CityParams{"us", ""}); }, ValidationException);
}

TEST(ResidentialServiceTest, IspsDecodesListPayload) {
  MockServer server;
  server.server().Post("/v1/proxy/city_isp", [](const httplib::Request&, httplib::Response& res) {
    res.set_content(
        R"({"code":0,"msg":"ok","data":{"list":[{"asn":"AS123","show_name":"ISP One"}]}})",
        "application/json");
  });

  auto transport = make_test_transport(server);
  ResidentialService service(transport);
  auto out = service.isps("us");

  ASSERT_EQ(out.list.size(), 1u);
  EXPECT_EQ(out.list[0].asn, "AS123");
  EXPECT_EQ(out.list[0].show_name, "ISP One");
}

TEST(ResidentialServiceTest, BalanceDecodesFlowBalance) {
  MockServer server;
  server.on_post_json("/v1/residential_flow/balance",
                      R"({"code":0,"msg":"ok","data":{"balance":5000,"expire_time":1732000000}})");

  auto transport = make_test_transport(server);
  ResidentialService service(transport);
  auto out = service.balance();

  EXPECT_EQ(out.balance, 5000);
  EXPECT_EQ(out.expire_time, 1732000000);
}

TEST(ResidentialServiceTest, ConsumeLogSendsTimeRangeAndDecodesList) {
  MockServer server;
  std::string start_value;
  std::string end_value;
  server.server().Post(
      "/v1/residential_flow/consume_log", [&](const httplib::Request& req, httplib::Response& res) {
        start_value = req.get_file_value("start_time").content;
        end_value = req.get_file_value("end_time").content;
        res.set_content(R"({"code":0,"msg":"ok","data":{"list":[{"id":1,"uid":2,)"
                        R"("balance":3,"all_buy":4,"use":5,"day":6,"expire_flow":7}]}})",
                        "application/json");
      });

  auto transport = make_test_transport(server);
  ResidentialService service(transport);
  auto out = service.consume_log(TimeRange{"2024-01-01 00:00:00", "2024-01-02 00:00:00"});

  EXPECT_EQ(start_value, "2024-01-01 00:00:00");
  EXPECT_EQ(end_value, "2024-01-02 00:00:00");
  ASSERT_EQ(out.list.size(), 1u);
  EXPECT_EQ(out.list[0].use, 5);
  EXPECT_EQ(out.list[0].expire_flow, 7);
}

TEST(ResidentialServiceTest, ConsumeLogMissingBoundsThrowsValidationException) {
  MockServer server;
  auto transport = make_test_transport(server);
  ResidentialService service(transport);
  try {
    (void)service.consume_log(TimeRange{"", ""});
    FAIL() << "expected ValidationException";
  } catch (const ValidationException& e) {
    ASSERT_EQ(e.fields().size(), 2u);
    EXPECT_EQ(e.fields()[0], "start_time");
    EXPECT_EQ(e.fields()[1], "end_time");
  }
}

}  // namespace
