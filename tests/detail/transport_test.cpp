#include "novada/detail/transport.hpp"

#include <atomic>
#include <chrono>
#include <map>
#include <string>

#include <gtest/gtest.h>

#include "novada/errors.hpp"
#include "support/mock_server.hpp"

namespace {

using novada::detail::Transport;
using novada::test::MockServer;

constexpr std::chrono::milliseconds kTestTimeout{2000};

Transport make_transport(const MockServer& server, int max_retries = 2) {
  return {"test-key",   server.base_url(), server.base_url(),    server.base_url(),
          kTestTimeout, max_retries,       "novada-cpp-test/0.0"};
}

TEST(TransportMultipartTest, DecodesEnvelopeInjectsBearerAndOmitsEmptyFields) {
  MockServer server;
  std::string seen_auth;
  std::string seen_content_type;
  bool product_seen = false;
  std::string product_value;
  bool ip_seen = false;

  server.server().Post("/v1/white_list/list", [&](const httplib::Request& req,
                                                  httplib::Response& res) {
    seen_auth = req.get_header_value("Authorization");
    seen_content_type = req.get_header_value("Content-Type");
    product_seen = req.has_file("product");
    if (product_seen) {
      product_value = req.get_file_value("product").content;
    }
    ip_seen = req.has_file("ip");  // must be omitted: empty optional field
    res.set_content(R"({"code":0,"msg":"success","data":{"list":[{"id":1,"mark_ip":"1.1.1.1"}],)"
                    R"("total":1},"timestamp":1732084616})",
                    "application/json");
  });

  Transport transport = make_transport(server);
  std::map<std::string, std::string> fields{{"product", "1"}, {"ip", ""}};
  nlohmann::json data = transport.do_multipart(server.base_url(), "/v1/white_list/list", fields);

  EXPECT_EQ(seen_auth, "Bearer test-key");
  EXPECT_EQ(seen_content_type.rfind("multipart/form-data", 0), 0u);
  EXPECT_TRUE(product_seen);
  EXPECT_EQ(product_value, "1");
  EXPECT_FALSE(ip_seen) << "empty optional fields must be omitted from the request body";
  ASSERT_EQ(data.at("total").get<int>(), 1);
  EXPECT_EQ(data.at("list")[0].at("mark_ip").get<std::string>(), "1.1.1.1");
}

TEST(TransportMultipartTest, NonZeroCodeThrowsApiException) {
  MockServer server;
  server.server().Post("/v1/white_list/add", [](const httplib::Request&, httplib::Response& res) {
    res.set_content(R"({"code":1001,"msg":"bad product","data":null})", "application/json");
  });

  Transport transport = make_transport(server);
  try {
    transport.do_multipart(server.base_url(), "/v1/white_list/add", {{"product", "1"}});
    FAIL() << "expected ApiException";
  } catch (const novada::ApiException& e) {
    EXPECT_EQ(e.code(), 1001);
    EXPECT_EQ(e.message(), "bad product");
    EXPECT_EQ(e.http_status(), 200);
  }
}

TEST(TransportMultipartTest, Http401MapsToAuthException) {
  MockServer server;
  server.server().Post("/v1/wallet/balance", [](const httplib::Request&, httplib::Response& res) {
    res.status = 401;
    res.set_content(R"({"msg":"unauthorized"})", "application/json");
  });

  Transport transport = make_transport(server);
  try {
    transport.do_multipart(server.base_url(), "/v1/wallet/balance", {{"page", "1"}});
    FAIL() << "expected AuthException";
  } catch (const novada::AuthException& e) {
    EXPECT_EQ(e.http_status(), 401);
  }
}

TEST(TransportMultipartTest, Http403MapsToAuthException) {
  MockServer server;
  server.server().Post("/v1/wallet/balance", [](const httplib::Request&, httplib::Response& res) {
    res.status = 403;
    res.set_content(R"({"msg":"forbidden"})", "application/json");
  });

  Transport transport = make_transport(server);
  EXPECT_THROW(
      { transport.do_multipart(server.base_url(), "/v1/wallet/balance", {{"page", "1"}}); },
      novada::AuthException);
}

// Note: wallet.Balance()-style calls (see the Go SDK's wallet.Service.Balance)
// send a genuinely empty fields map. libcurl's curl_mime with zero parts
// serializes to just the closing boundary line ("--BOUNDARY--\r\n", no
// opening boundary or part), matching Go's mime/multipart.Writer.Close() with
// no fields written (verified manually against a raw socket capture). This
// specific edge case cannot be exercised through MockServer: cpp-httplib's
// Server::routing() always runs the request through
// MultipartFormDataParser for a multipart/form-data Content-Type regardless
// of handler type, and that parser requires at least one opening boundary
// line to consider the body valid, so it 400s before any handler (including
// a raw HandlerWithContentReader) is invoked. That is a mock-server parser
// limitation, not a real-server requirement -- Go's SDK already relies on
// this exact wire format working against the production API.

TEST(TransportMultipartTest, Retries429ThenSucceeds) {
  MockServer server;
  std::atomic<int> call_count{0};
  server.server().Post("/v1/white_list/list", [&](const httplib::Request&, httplib::Response& res) {
    int n = ++call_count;
    if (n < 3) {
      res.status = 429;
      res.set_content(R"({"msg":"slow down"})", "application/json");
      return;
    }
    res.set_content(R"({"code":0,"msg":"ok","data":{"list":[],"total":0}})", "application/json");
  });

  Transport transport = make_transport(server, /*max_retries=*/2);
  nlohmann::json data =
      transport.do_multipart(server.base_url(), "/v1/white_list/list", {{"product", "1"}});
  EXPECT_EQ(call_count.load(), 3);
  EXPECT_EQ(data.at("total").get<int>(), 0);
}

TEST(TransportMultipartTest, DoesNotRetryBusinessCodeFailure) {
  MockServer server;
  std::atomic<int> call_count{0};
  server.server().Post("/v1/white_list/add", [&](const httplib::Request&, httplib::Response& res) {
    ++call_count;
    res.set_content(R"({"code":1001,"msg":"bad product","data":null})", "application/json");
  });

  Transport transport = make_transport(server, /*max_retries=*/2);
  EXPECT_THROW(
      { transport.do_multipart(server.base_url(), "/v1/white_list/add", {{"product", "1"}}); },
      novada::ApiException);
  EXPECT_EQ(call_count.load(), 1) << "business code != 0 must never be retried";
}

TEST(TransportMultipartTest, ExhaustsRetriesOn5xxThenThrows) {
  MockServer server;
  std::atomic<int> call_count{0};
  server.server().Post("/v1/white_list/list", [&](const httplib::Request&, httplib::Response& res) {
    ++call_count;
    res.status = 503;
    res.set_content("Service Unavailable", "text/plain");
  });

  Transport transport = make_transport(server, /*max_retries=*/1);
  try {
    transport.do_multipart(server.base_url(), "/v1/white_list/list", {{"product", "1"}});
    FAIL() << "expected ApiException";
  } catch (const novada::ApiException& e) {
    EXPECT_EQ(e.http_status(), 503);
  }
  EXPECT_EQ(call_count.load(), 2) << "max_retries=1 means 2 total attempts";
}

TEST(TransportMultipartTest, RawVariantReturnsBodyWithoutEnvelopeDecoding) {
  MockServer server;
  const std::string file_bytes = "id,ip\n1,1.1.1.1\n";
  server.server().Post("/v1/static/export", [&](const httplib::Request&, httplib::Response& res) {
    res.set_content(file_bytes, "text/csv");
  });

  Transport transport = make_transport(server);
  std::string body =
      transport.do_multipart_raw(server.base_url(), "/v1/static/export", {{"id", "1"}});
  EXPECT_EQ(body, file_bytes);
}

TEST(TransportFormUrlencodedTest, EncodesFieldsAndDecodesEnvelope) {
  MockServer server;
  std::string seen_content_type;
  std::string seen_body;
  server.server().Post("/request", [&](const httplib::Request& req, httplib::Response& res) {
    seen_content_type = req.get_header_value("Content-Type");
    seen_body = req.body;
    res.set_content(R"({"code":0,"msg":"ok","data":{"balance":100}})", "application/json");
  });

  Transport transport = make_transport(server);
  std::map<std::string, std::string> fields{{"scraper_name", "google_search"},
                                            {"scraper_params", R"({"q":"a b"})"}};
  nlohmann::json data = transport.do_form_urlencoded(server.base_url(), "/request", fields);

  EXPECT_EQ(seen_content_type, "application/x-www-form-urlencoded");
  EXPECT_NE(seen_body.find("scraper_name=google_search"), std::string::npos);
  EXPECT_NE(seen_body.find("scraper_params=%7B%22q%22%3A%22a%20b%22%7D"), std::string::npos);
  EXPECT_EQ(data.at("balance").get<int>(), 100);
}

TEST(TransportFormUrlencodedTest, RawVariantReturnsScrapedPayloadAsIs) {
  MockServer server;
  const std::string scraped = R"({"not":"an envelope"})";
  server.server().Post("/request", [&](const httplib::Request&, httplib::Response& res) {
    res.set_content(scraped, "application/json");
  });

  Transport transport = make_transport(server);
  std::string body =
      transport.do_form_urlencoded_raw(server.base_url(), "/request", {{"scraper_name", "x"}});
  EXPECT_EQ(body, scraped);
}

}  // namespace
