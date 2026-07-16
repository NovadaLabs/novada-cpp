#include "novada/proxy/mobile_service.hpp"

#include <string>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "novada/errors.hpp"
#include "support/mock_server.hpp"
#include "support/service_test_util.hpp"

namespace {

using novada::ValidationException;
using novada::proxy::MobileService;
using novada::proxy::MobileUseParams;
using novada::test::make_test_transport;
using novada::test::MockServer;

TEST(MobileServiceTest, CountriesReturnsRawDataJsonString) {
  MockServer server;
  server.on_post_json("/v1/proxy/mobile_area",
                      R"({"code":0,"msg":"ok","data":{"anything":[1,2,3],"x":"y"}})");

  auto transport = make_test_transport(server);
  MobileService service(transport);
  std::string raw = service.countries();

  // The raw string is the JSON of the data payload; parse it back to assert
  // its content without depending on key ordering.
  nlohmann::json parsed = nlohmann::json::parse(raw);
  EXPECT_EQ(parsed.at("anything"), (nlohmann::json{1, 2, 3}));
  EXPECT_EQ(parsed.at("x").get<std::string>(), "y");
}

TEST(MobileServiceTest, BalanceDecodesMobileBalance) {
  MockServer server;
  server.on_post_json("/v1/mobile_flow/mobile_flow_balance",
                      R"({"code":0,"msg":"ok","data":{"balance":8888}})");

  auto transport = make_test_transport(server);
  MobileService service(transport);
  auto out = service.balance();
  EXPECT_EQ(out.balance, 8888);
}

TEST(MobileServiceTest, ConsumeLogSendsGranularityAsDayOrHour) {
  MockServer server;
  std::string start_value;
  std::string end_value;
  std::string granularity_value;
  server.server().Post(
      "/v1/mobile_flow/mobile_flow_use", [&](const httplib::Request& req, httplib::Response& res) {
        start_value = req.get_file_value("start_time").content;
        end_value = req.get_file_value("end_time").content;
        granularity_value = req.get_file_value("day_or_hour").content;
        res.set_content(R"({"code":0,"msg":"ok","data":{"list":[]}})", "application/json");
      });

  auto transport = make_test_transport(server);
  MobileService service(transport);
  (void)service.consume_log(MobileUseParams{"2024-01-01 00:00:00", "2024-01-02 00:00:00", "2"});

  EXPECT_EQ(start_value, "2024-01-01 00:00:00");
  EXPECT_EQ(end_value, "2024-01-02 00:00:00");
  EXPECT_EQ(granularity_value, "2");
}

TEST(MobileServiceTest, ConsumeLogMissingGranularityThrowsValidationException) {
  MockServer server;
  auto transport = make_test_transport(server);
  MobileService service(transport);
  try {
    (void)service.consume_log(MobileUseParams{"2024-01-01 00:00:00", "2024-01-02 00:00:00", ""});
    FAIL() << "expected ValidationException";
  } catch (const ValidationException& e) {
    ASSERT_EQ(e.fields().size(), 1u);
    EXPECT_EQ(e.fields()[0], "day_or_hour");
  }
}

}  // namespace
