#include "novada/scraper/api_service.hpp"

#include <string>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "novada/errors.hpp"
#include "novada/scraper/scraper_service.hpp"
#include "support/mock_server.hpp"
#include "support/service_test_util.hpp"

namespace {

using novada::ValidationException;
using novada::scraper::GoogleSearchParams;
using novada::scraper::ScraperService;
using novada::scraper::YouTubeVideoParams;
using novada::test::make_test_transport;
using novada::test::MockServer;

TEST(YouTubeServiceTest, VideoPostBuildsScraperParamsAndReturnsRaw) {
  MockServer server;
  std::string scraper_name;
  std::string scraper_id;
  std::string scraper_params;
  server.server().Post("/request", [&](const httplib::Request& req, httplib::Response& res) {
    scraper_name = req.get_param_value("scraper_name");
    scraper_id = req.get_param_value("scraper_id");
    scraper_params = req.get_param_value("scraper_params");
    res.set_content("YT-RAW", "application/json");
  });

  auto transport = make_test_transport(server);
  ScraperService service(transport);
  auto response = service.api().youtube().video_post(YouTubeVideoParams{"https://youtu.be/x"});

  EXPECT_EQ(scraper_name, "youtube.com");
  EXPECT_EQ(scraper_id, "youtube_video-post_explore");
  nlohmann::json parsed = nlohmann::json::parse(scraper_params);
  ASSERT_EQ(parsed.size(), 1u);
  EXPECT_EQ(parsed[0].at("url").get<std::string>(), "https://youtu.be/x");
  EXPECT_EQ(response.raw, "YT-RAW");
}

TEST(GoogleServiceTest, SearchSendsFlatFormFields) {
  MockServer server;
  std::string q;
  std::string json_flag;
  std::string device;
  std::string render_js;
  bool no_cache_seen = false;
  server.server().Post("/request", [&](const httplib::Request& req, httplib::Response& res) {
    q = req.get_param_value("q");
    json_flag = req.get_param_value("json");
    device = req.get_param_value("device");
    render_js = req.get_param_value("render_js");
    no_cache_seen = req.has_param("no_cache");
    res.set_content(R"({"code":0,"msg":"ok","data":{"code":200,"data":{}}})", "application/json");
  });

  auto transport = make_test_transport(server);
  ScraperService service(transport);
  GoogleSearchParams params;
  params.query = "novada";
  params.device = "desktop";
  params.render_js = true;
  (void)service.api().google().search(params);

  EXPECT_EQ(q, "novada");
  EXPECT_EQ(json_flag, "1");  // default JSON output
  EXPECT_EQ(device, "desktop");
  EXPECT_EQ(render_js, "true");
  EXPECT_FALSE(no_cache_seen) << "unset optional<bool> must be omitted";
}

TEST(GoogleServiceTest, SearchDecodesStructuredResult) {
  MockServer server;
  server.server().Post("/request", [](const httplib::Request&, httplib::Response& res) {
    res.set_content(R"({"code":0,"msg":"success","data":{"code":200,"cost_time":123,"msg":"ok",)"
                    R"("data":{"filename":"f.json","json":[{"title":"result"}],"task_id":"t1"}}})",
                    "application/json");
  });

  auto transport = make_test_transport(server);
  ScraperService service(transport);
  GoogleSearchParams params;
  params.query = "novada";
  auto out = service.api().google().search(params);

  EXPECT_EQ(out.code, 200);
  EXPECT_EQ(out.cost_time, 123);
  EXPECT_EQ(out.data.filename, "f.json");
  EXPECT_EQ(out.data.task_id, "t1");
  // The raw json array is preserved as text.
  nlohmann::json inner = nlohmann::json::parse(out.data.json);
  ASSERT_TRUE(inner.is_array());
  EXPECT_EQ(inner[0].at("title").get<std::string>(), "result");
}

TEST(GoogleServiceTest, HtmlOutputSetsJsonZero) {
  MockServer server;
  std::string json_flag;
  server.server().Post("/request", [&](const httplib::Request& req, httplib::Response& res) {
    json_flag = req.get_param_value("json");
    res.set_content(R"({"code":0,"msg":"ok","data":{"code":200,"data":{"html":"<html></html>"}}})",
                    "application/json");
  });

  auto transport = make_test_transport(server);
  ScraperService service(transport);
  GoogleSearchParams params;
  params.query = "novada";
  params.html = true;
  auto out = service.api().google().search(params);

  EXPECT_EQ(json_flag, "0");
  ASSERT_TRUE(out.data.html.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access) -- guarded by ASSERT_TRUE above.
  EXPECT_EQ(out.data.html.value(), "<html></html>");
}

TEST(GoogleServiceTest, MissingQueryThrowsValidationException) {
  MockServer server;
  auto transport = make_test_transport(server);
  ScraperService service(transport);
  try {
    (void)service.api().google().search(GoogleSearchParams{});
    FAIL() << "expected ValidationException";
  } catch (const ValidationException& e) {
    ASSERT_EQ(e.fields().size(), 1u);
    EXPECT_EQ(e.fields()[0], "q");
  }
}

TEST(GoogleServiceTest, NonZeroEnvelopeCodePropagatesAsApiException) {
  MockServer server;
  server.server().Post("/request", [](const httplib::Request&, httplib::Response& res) {
    res.set_content(R"({"code":2001,"msg":"quota exceeded","data":null})", "application/json");
  });

  auto transport = make_test_transport(server);
  ScraperService service(transport);
  GoogleSearchParams params;
  params.query = "novada";
  try {
    (void)service.api().google().search(params);
    FAIL() << "expected ApiException";
  } catch (const novada::ApiException& e) {
    EXPECT_EQ(e.code(), 2001);
  }
}

}  // namespace
