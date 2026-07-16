#include "novada/client.hpp"

#include <cstdlib>

#include <gtest/gtest.h>

#include "novada/errors.hpp"

namespace {

// Ensure NOVADA_API_KEY does not leak between tests.
class ClientTest : public ::testing::Test {
 protected:
  void TearDown() override {
#ifdef _WIN32
    _putenv_s("NOVADA_API_KEY", "");
#else
    unsetenv("NOVADA_API_KEY");
#endif
  }
};

void set_env_api_key(const std::string& value) {
#ifdef _WIN32
  _putenv_s("NOVADA_API_KEY", value.c_str());
#else
  setenv("NOVADA_API_KEY", value.c_str(), 1);
#endif
}

TEST_F(ClientTest, UsesDefaultBaseUrls) {
  novada::Client client("k");
  EXPECT_EQ(client.base_url(), novada::kDefaultBaseUrl);
  EXPECT_EQ(client.web_unblocker_url(), novada::kDefaultWebUnblockerUrl);
  EXPECT_EQ(client.scraper_url(), novada::kDefaultScraperUrl);
}

TEST_F(ClientTest, ThrowsWithoutApiKeyOrEnvVar) {
#ifdef _WIN32
  _putenv_s("NOVADA_API_KEY", "");
#else
  unsetenv("NOVADA_API_KEY");
#endif
  EXPECT_THROW({ novada::Client client(""); }, novada::NovadaException);
}

TEST_F(ClientTest, FallsBackToEnvironmentVariable) {
  set_env_api_key("env-key");
  EXPECT_NO_THROW({ novada::Client client(""); });
}

TEST_F(ClientTest, ExplicitApiKeyTakesPrecedenceOverEnv) {
  set_env_api_key("env-key");
  EXPECT_NO_THROW({ novada::Client client("explicit-key"); });
}

TEST_F(ClientTest, ClientOptionsChainingOverridesDefaults) {
  novada::ClientOptions opts;
  opts.set_base_url("https://base.example.com")
      .set_web_unblocker_url("https://unblocker.example.com")
      .set_scraper_url("https://scraper.example.com")
      .set_timeout(std::chrono::seconds(10))
      .set_max_retries(5)
      .set_user_agent("custom-agent/1.0");

  EXPECT_EQ(opts.timeout, std::chrono::milliseconds(10000));
  EXPECT_EQ(opts.max_retries, 5);
  EXPECT_EQ(opts.user_agent, "custom-agent/1.0");

  novada::Client client("k", opts);
  EXPECT_EQ(client.base_url(), "https://base.example.com");
  EXPECT_EQ(client.web_unblocker_url(), "https://unblocker.example.com");
  EXPECT_EQ(client.scraper_url(), "https://scraper.example.com");
}

TEST_F(ClientTest, IsMoveConstructible) {
  novada::Client client("k");
  novada::Client moved(std::move(client));
  EXPECT_EQ(moved.base_url(), novada::kDefaultBaseUrl);
}

}  // namespace
