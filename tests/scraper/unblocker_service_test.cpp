#include "novada/scraper/unblocker_service.hpp"

#include <string>

#include <gtest/gtest.h>

#include "novada/errors.hpp"
#include "novada/scraper/scraper_service.hpp"
#include "support/mock_server.hpp"
#include "support/service_test_util.hpp"

namespace {

using novada::ValidationException;
using novada::scraper::ScraperService;
using novada::scraper::UnblockerParams;
using novada::test::make_test_transport;
using novada::test::MockServer;

TEST(UnblockerServiceTest, ScrapeSendsFieldsAndDecodesResult) {
  MockServer server;
  std::string target_url;
  std::string response_format;
  std::string js_render;
  std::string wait_ms;
  bool country_seen = false;
  server.server().Post("/request", [&](const httplib::Request& req, httplib::Response& res) {
    target_url = req.get_param_value("target_url");
    response_format = req.get_param_value("response_format");
    js_render = req.get_param_value("js_render");
    wait_ms = req.get_param_value("wait_ms");
    country_seen = req.has_param("country");
    res.set_content(
        R"({"code":0,"msg":"success","data":{"code":200,"html":"<h1>hi</h1>","msg":"ok",)"
        R"("msg_detail":"","use_balance":0.5}})",
        "application/json");
  });

  auto transport = make_test_transport(server);
  ScraperService service(transport);
  UnblockerParams params;
  params.target_url = "https://example.com";
  params.js_render = true;
  params.wait_ms = 5000;
  auto out = service.unblocker().scrape(params);

  EXPECT_EQ(target_url, "https://example.com");
  EXPECT_EQ(response_format, "html");  // default
  EXPECT_EQ(js_render, "true");
  EXPECT_EQ(wait_ms, "5000");
  EXPECT_FALSE(country_seen);

  EXPECT_EQ(out.code, 200);
  EXPECT_EQ(out.html, "<h1>hi</h1>");
  EXPECT_DOUBLE_EQ(out.use_balance, 0.5);
}

TEST(UnblockerServiceTest, ScrapeOmitsWaitMsWhenZeroAndHonorsResponseFormat) {
  MockServer server;
  bool wait_ms_seen = false;
  std::string response_format;
  server.server().Post("/request", [&](const httplib::Request& req, httplib::Response& res) {
    wait_ms_seen = req.has_param("wait_ms");
    response_format = req.get_param_value("response_format");
    res.set_content(R"({"code":0,"msg":"ok","data":{"code":200}})", "application/json");
  });

  auto transport = make_test_transport(server);
  ScraperService service(transport);
  UnblockerParams params;
  params.target_url = "https://example.com";
  params.response_format = "html,png";
  auto out = service.unblocker().scrape(params);

  EXPECT_FALSE(wait_ms_seen) << "wait_ms must be omitted when not positive";
  EXPECT_EQ(response_format, "html,png");
  EXPECT_EQ(out.code, 200);
}

TEST(UnblockerServiceTest, ScrapeUsesWebUnblockerHost) {
  MockServer scraper_host;
  MockServer unblocker_host;
  bool unblocker_hit = false;
  unblocker_host.server().Post("/request", [&](const httplib::Request&, httplib::Response& res) {
    unblocker_hit = true;
    res.set_content(R"({"code":0,"msg":"ok","data":{"code":200}})", "application/json");
  });

  novada::detail::Transport transport("test-key", scraper_host.base_url(),
                                      unblocker_host.base_url(), scraper_host.base_url(),
                                      std::chrono::milliseconds(2000), 2, "novada-cpp-test/0.0");
  ScraperService service(transport);
  UnblockerParams params;
  params.target_url = "https://example.com";
  (void)service.unblocker().scrape(params);

  EXPECT_TRUE(unblocker_hit);
}

TEST(UnblockerServiceTest, ScrapeMissingTargetUrlThrowsValidationException) {
  MockServer server;
  auto transport = make_test_transport(server);
  ScraperService service(transport);
  EXPECT_THROW({ (void)service.unblocker().scrape(UnblockerParams{}); }, ValidationException);
}

TEST(UnblockerServiceTest, CountriesDecodesAreas) {
  MockServer server;
  server.on_post_json(
      "/v1/proxy/unblocker_area",
      R"({"code":0,"msg":"ok","data":{"continent":{"1":"Asia"},"country":[{"continent":"Asia",)"
      R"("continent_code":1,"list":[{"code":"hk","name_en":"Hong Kong"}]}]}})");

  auto transport = make_test_transport(server);
  ScraperService service(transport);
  auto out = service.unblocker().countries();

  EXPECT_EQ(out.continent.at("1"), "Asia");
  ASSERT_EQ(out.country.size(), 1u);
  ASSERT_EQ(out.country[0].list.size(), 1u);
  EXPECT_EQ(out.country[0].list[0].code, "hk");
}

TEST(UnblockerServiceTest, StatesSendsCodeAndDecodes) {
  MockServer server;
  std::string code_value;
  server.server().Post("/v1/proxy/unblocker_area_by_country", [&](const httplib::Request& req,
                                                                  httplib::Response& res) {
    code_value = req.get_file_value("code").content;
    res.set_content(R"({"code":0,"msg":"ok","data":{"data":[{"state":"hialeah"}]}})",
                    "application/json");
  });

  auto transport = make_test_transport(server);
  ScraperService service(transport);
  auto out = service.unblocker().states("us");

  EXPECT_EQ(code_value, "us");
  ASSERT_EQ(out.data.size(), 1u);
  EXPECT_EQ(out.data[0].state, "hialeah");
}

TEST(UnblockerServiceTest, CitiesRequiresCodeAndRegion) {
  MockServer server;
  auto transport = make_test_transport(server);
  ScraperService service(transport);
  try {
    (void)service.unblocker().cities("", "");
    FAIL() << "expected ValidationException";
  } catch (const ValidationException& e) {
    ASSERT_EQ(e.fields().size(), 2u);
    EXPECT_EQ(e.fields()[0], "code");
    EXPECT_EQ(e.fields()[1], "region");
  }
}

TEST(UnblockerServiceTest, IspsDecodesCarrierList) {
  MockServer server;
  server.server().Post(
      "/v1/proxy/unblocker_city_isp", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"code":0,"msg":"ok","data":{"list":[{"asn":"AS327712",)"
                        R"("show_name":"Telecom Algeria"}]}})",
                        "application/json");
      });

  auto transport = make_test_transport(server);
  ScraperService service(transport);
  auto out = service.unblocker().isps("dz");

  ASSERT_EQ(out.list.size(), 1u);
  EXPECT_EQ(out.list[0].asn, "AS327712");
  EXPECT_EQ(out.list[0].show_name, "Telecom Algeria");
}

}  // namespace
