#include <string>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "novada/errors.hpp"
#include "novada/scraper/browser_service.hpp"
#include "novada/scraper/scraper_service.hpp"
#include "novada/scraper/universal_service.hpp"
#include "support/mock_server.hpp"
#include "support/service_test_util.hpp"

namespace {

using novada::ValidationException;
using novada::scraper::ScraperService;
using novada::test::make_test_transport;
using novada::test::MockServer;

TEST(UniversalServiceTest, BalanceDecodesScraperBalanceObject) {
  MockServer server;
  server.on_post_json("/v1/capture/get_balance",
                      R"({"code":0,"msg":"ok","data":{"scraper_balance":17056}})");

  auto transport = make_test_transport(server);
  ScraperService service(transport);
  auto out = service.universal().balance();
  EXPECT_DOUBLE_EQ(out.scraper_balance, 17056);
}

TEST(UniversalServiceTest, BalanceAcceptsBareNumberFallback) {
  MockServer server;
  // The Go SDK decoded a bare number; the decoder accepts it for robustness.
  server.on_post_json("/v1/capture/get_balance", R"({"code":0,"msg":"ok","data":42.5})");

  auto transport = make_test_transport(server);
  ScraperService service(transport);
  auto out = service.universal().balance();
  EXPECT_DOUBLE_EQ(out.scraper_balance, 42.5);
}

TEST(UniversalServiceTest, LogsDecodesRecords) {
  MockServer server;
  server.on_post_json("/v1/capture/logs",
                      R"({"code":0,"msg":"ok","data":{"list":[{"time_label":"2026-03-26",)"
                      R"("unlocker_total_cost":1.5,"scraper_total_cost":2.5,"scraper_used_res":10,)"
                      R"("browser_used_flow":100}]}})");

  auto transport = make_test_transport(server);
  ScraperService service(transport);
  auto out = service.universal().logs();

  ASSERT_EQ(out.list.size(), 1u);
  EXPECT_EQ(out.list[0].time_label, "2026-03-26");
  EXPECT_DOUBLE_EQ(out.list[0].unlocker_total_cost, 1.5);
  EXPECT_EQ(out.list[0].scraper_used_res, 10);
  EXPECT_EQ(out.list[0].browser_used_flow, 100);
}

TEST(UniversalServiceTest, UnitDecodesGroupedPrices) {
  MockServer server;
  server.on_post_json(
      "/v1/capture/unit",
      R"({"code":0,"msg":"ok","data":{"scraper":[{"package":"scraper","level":1,"price":1.6,)"
      R"("available":0}],"unblocker":[{"package":"unblocker","level":2,"price":3.2,"available":1}]}})");

  auto transport = make_test_transport(server);
  ScraperService service(transport);
  auto out = service.universal().unit();

  ASSERT_EQ(out.scraper.size(), 1u);
  EXPECT_EQ(out.scraper[0].package, "scraper");
  EXPECT_DOUBLE_EQ(out.scraper[0].price, 1.6);
  ASSERT_EQ(out.unblocker.size(), 1u);
  EXPECT_EQ(out.unblocker[0].level, 2);
  EXPECT_EQ(out.unblocker[0].available, 1);
}

TEST(BrowserServiceTest, CountriesReturnsRawDataJson) {
  MockServer server;
  server.on_post_json("/v1/proxy/browser_area",
                      R"({"code":0,"msg":"ok","data":{"some":"shape","n":5}})");

  auto transport = make_test_transport(server);
  ScraperService service(transport);
  std::string raw = service.browser().countries();

  nlohmann::json parsed = nlohmann::json::parse(raw);
  EXPECT_EQ(parsed.at("some").get<std::string>(), "shape");
  EXPECT_EQ(parsed.at("n").get<int>(), 5);
}

TEST(BrowserServiceTest, FlowUseSendsTimeRangeAndDecodes) {
  MockServer server;
  std::string start_value;
  std::string end_value;
  server.server().Post("/v1/browser_flow/browser_flow_use", [&](const httplib::Request& req,
                                                                httplib::Response& res) {
    start_value = req.get_file_value("start_time").content;
    end_value = req.get_file_value("end_time").content;
    res.set_content(R"({"code":0,"msg":"ok","data":{"list":[{"id":1,"uid":81,)"
                    R"("balance":30999907705,"all_buy":31000000000,"use":92295,)"
                    R"("day":1731427200,"expire_flow":0}]}})",
                    "application/json");
  });

  auto transport = make_test_transport(server);
  ScraperService service(transport);
  auto out = service.browser().flow_use("2024-01-01 00:00:00", "2024-01-02 00:00:00");

  EXPECT_EQ(start_value, "2024-01-01 00:00:00");
  EXPECT_EQ(end_value, "2024-01-02 00:00:00");
  ASSERT_EQ(out.list.size(), 1u);
  EXPECT_EQ(out.list[0].balance, 30999907705LL);
  EXPECT_EQ(out.list[0].use, 92295);
}

TEST(BrowserServiceTest, FlowUseMissingBoundsThrowsValidationException) {
  MockServer server;
  auto transport = make_test_transport(server);
  ScraperService service(transport);
  EXPECT_THROW({ (void)service.browser().flow_use("", ""); }, ValidationException);
}

}  // namespace
