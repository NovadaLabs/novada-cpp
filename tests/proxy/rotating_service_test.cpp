#include "novada/proxy/rotating_service.hpp"

#include <string>

#include <gtest/gtest.h>

#include "novada/errors.hpp"
#include "support/mock_server.hpp"
#include "support/service_test_util.hpp"

namespace {

using novada::ValidationException;
using novada::proxy::RotatingDCService;
using novada::proxy::RotatingISPService;
using novada::proxy::TimeRange;
using novada::test::make_test_transport;
using novada::test::MockServer;

TEST(RotatingISPServiceTest, CountriesDecodesCountryList) {
  MockServer server;
  server.on_post_json("/v1/proxy/isp_data_area",
                      R"({"code":0,"msg":"ok","data":{"country":[{"code":"us",)"
                      R"("node":"node1"},{"code":"uk","node":"node2"}]}})");

  auto transport = make_test_transport(server);
  RotatingISPService service(transport);
  auto out = service.countries();

  ASSERT_EQ(out.country.size(), 2u);
  EXPECT_EQ(out.country[0].code, "us");
  EXPECT_EQ(out.country[0].node, "node1");
  EXPECT_EQ(out.country[1].code, "uk");
}

TEST(RotatingISPServiceTest, BalanceAndConsumeLogUseCorrectPaths) {
  MockServer server;
  server.on_post_json("/v1/isp_flow/balance", R"({"code":0,"msg":"ok","data":{"balance":42}})");
  bool consume_hit = false;
  server.server().Post(
      "/v1/isp_flow/consume_log", [&](const httplib::Request&, httplib::Response& res) {
        consume_hit = true;
        res.set_content(R"({"code":0,"msg":"ok","data":{"list":[]}})", "application/json");
      });

  auto transport = make_test_transport(server);
  RotatingISPService service(transport);
  EXPECT_EQ(service.balance().balance, 42);
  (void)service.consume_log(TimeRange{"2024-01-01 00:00:00", "2024-01-02 00:00:00"});
  EXPECT_TRUE(consume_hit);
}

TEST(RotatingDCServiceTest, CountriesDecodesNestedCity) {
  MockServer server;
  server.on_post_json("/v1/proxy/dynamic_data_area",
                      R"({"code":0,"msg":"ok","data":{"country":[{"id":7,"code":"jp",)"
                      R"("status":1,"continents":2,"area_code":81,"name_en":"Japan",)"
                      R"("available":1,"city":{"code":"tokyo"}}]}})");

  auto transport = make_test_transport(server);
  RotatingDCService service(transport);
  auto out = service.countries();

  ASSERT_EQ(out.country.size(), 1u);
  EXPECT_EQ(out.country[0].id, 7);
  EXPECT_EQ(out.country[0].code, "jp");
  EXPECT_EQ(out.country[0].name_en, "Japan");
  EXPECT_EQ(out.country[0].city.code, "tokyo");
}

TEST(RotatingDCServiceTest, ConsumeLogValidatesBounds) {
  MockServer server;
  auto transport = make_test_transport(server);
  RotatingDCService service(transport);
  EXPECT_THROW(
      { (void)service.consume_log(TimeRange{"", "2024-01-02 00:00:00"}); }, ValidationException);
}

}  // namespace
