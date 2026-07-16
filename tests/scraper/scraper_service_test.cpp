#include "novada/scraper/scraper_service.hpp"

#include <string>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "novada/errors.hpp"
#include "support/mock_server.hpp"
#include "support/service_test_util.hpp"

namespace {

using novada::ValidationException;
using novada::scraper::ScrapeRequest;
using novada::scraper::ScraperService;
using novada::scraper::Target;
using novada::test::make_test_transport;
using novada::test::MockServer;

ScrapeRequest sample_request() {
  ScrapeRequest req;
  req.target = Target::kScraperAPI;
  req.scraper_name = "youtube.com";
  req.scraper_id = "youtube_video-post_explore";
  req.params = {{{"url", "https://youtu.be/x"}}};
  return req;
}

TEST(ScraperDoRequestTest, EncodesFormFieldsAndSerializesParams) {
  MockServer server;
  std::string content_type;
  std::string scraper_name;
  std::string scraper_id;
  std::string scraper_params;
  bool scraper_errors_seen = false;
  server.server().Post("/request", [&](const httplib::Request& req, httplib::Response& res) {
    content_type = req.get_header_value("Content-Type");
    scraper_name = req.get_param_value("scraper_name");
    scraper_id = req.get_param_value("scraper_id");
    scraper_params = req.get_param_value("scraper_params");
    scraper_errors_seen = req.has_param("scraper_errors");
    res.set_content("RAW-BODY", "application/json");
  });

  auto transport = make_test_transport(server);
  ScraperService service(transport);
  auto response = service.do_request(sample_request());

  EXPECT_EQ(content_type, "application/x-www-form-urlencoded");
  EXPECT_EQ(scraper_name, "youtube.com");
  EXPECT_EQ(scraper_id, "youtube_video-post_explore");
  // scraper_params is the JSON array of the params list.
  nlohmann::json parsed = nlohmann::json::parse(scraper_params);
  ASSERT_TRUE(parsed.is_array());
  ASSERT_EQ(parsed.size(), 1u);
  EXPECT_EQ(parsed[0].at("url").get<std::string>(), "https://youtu.be/x");
  EXPECT_FALSE(scraper_errors_seen);
  EXPECT_EQ(response.raw, "RAW-BODY");
}

TEST(ScraperDoRequestTest, ReturnErrorsSetsScraperErrorsField) {
  MockServer server;
  std::string scraper_errors;
  server.server().Post("/request", [&](const httplib::Request& req, httplib::Response& res) {
    scraper_errors = req.get_param_value("scraper_errors");
    res.set_content("{}", "application/json");
  });

  auto transport = make_test_transport(server);
  ScraperService service(transport);
  ScrapeRequest req = sample_request();
  req.return_errors = true;
  (void)service.do_request(req);

  EXPECT_EQ(scraper_errors, "true");
}

TEST(ScraperDoRequestTest, RawBodyIsReturnedWithoutEnvelopeDecoding) {
  MockServer server;
  // A body that is NOT a valid envelope must still be returned verbatim.
  const std::string csv = "a,b\n1,2\n";
  server.server().Post("/request", [&](const httplib::Request&, httplib::Response& res) {
    res.set_content(csv, "text/csv");
  });

  auto transport = make_test_transport(server);
  ScraperService service(transport);
  auto response = service.do_request(sample_request());
  EXPECT_EQ(response.raw, csv);
}

TEST(ScraperDoRequestTest, MissingRequiredFieldsThrowValidationException) {
  MockServer server;
  auto transport = make_test_transport(server);
  ScraperService service(transport);

  ScrapeRequest req;  // all empty
  try {
    (void)service.do_request(req);
    FAIL() << "expected ValidationException";
  } catch (const ValidationException& e) {
    ASSERT_EQ(e.fields().size(), 3u);
    EXPECT_EQ(e.fields()[0], "scraper_name");
    EXPECT_EQ(e.fields()[1], "scraper_id");
    EXPECT_EQ(e.fields()[2], "scraper_params");
  }
}

TEST(ScraperDoRequestTest, EmptyParamsListIsRejected) {
  MockServer server;
  auto transport = make_test_transport(server);
  ScraperService service(transport);

  ScrapeRequest req;
  req.scraper_name = "youtube.com";
  req.scraper_id = "youtube_video-post_explore";
  req.params = {};  // empty
  try {
    (void)service.do_request(req);
    FAIL() << "expected ValidationException";
  } catch (const ValidationException& e) {
    ASSERT_EQ(e.fields().size(), 1u);
    EXPECT_EQ(e.fields()[0], "scraper_params");
  }
}

TEST(ScraperDoRequestTest, TargetSelectsScraperVersusWebUnblockerHost) {
  MockServer scraper_host;
  MockServer unblocker_host;
  bool scraper_hit = false;
  bool unblocker_hit = false;
  scraper_host.server().Post("/request", [&](const httplib::Request&, httplib::Response& res) {
    scraper_hit = true;
    res.set_content("from-scraper", "text/plain");
  });
  unblocker_host.server().Post("/request", [&](const httplib::Request&, httplib::Response& res) {
    unblocker_hit = true;
    res.set_content("from-unblocker", "text/plain");
  });

  // base_url points at the scraper host (unused here); scraper_url and
  // web_unblocker_url point at the two distinct hosts.
  novada::detail::Transport transport("test-key", scraper_host.base_url(),
                                      unblocker_host.base_url(), scraper_host.base_url(),
                                      std::chrono::milliseconds(2000), 2, "novada-cpp-test/0.0");
  ScraperService service(transport);

  ScrapeRequest req = sample_request();
  req.target = Target::kWebUnblocker;
  auto unblocker_response = service.do_request(req);
  EXPECT_TRUE(unblocker_hit);
  EXPECT_FALSE(scraper_hit);
  EXPECT_EQ(unblocker_response.raw, "from-unblocker");

  req.target = Target::kScraperAPI;
  auto scraper_response = service.do_request(req);
  EXPECT_TRUE(scraper_hit);
  EXPECT_EQ(scraper_response.raw, "from-scraper");
}

}  // namespace
