#include "novada/errors.hpp"

#include <gtest/gtest.h>

namespace {

using novada::ApiException;
using novada::AuthException;
using novada::NovadaException;
using novada::RateLimitException;
using novada::ValidationException;

TEST(ApiExceptionTest, MessageIncludesCodeWhenNonZero) {
  ApiException e(200, 1001, "bad product");
  EXPECT_EQ(e.http_status(), 200);
  EXPECT_EQ(e.code(), 1001);
  EXPECT_EQ(e.message(), "bad product");
  EXPECT_NE(std::string(e.what()).find("code=1001"), std::string::npos);
  EXPECT_NE(std::string(e.what()).find("http=200"), std::string::npos);
}

TEST(ApiExceptionTest, MessageOmitsCodeWhenZero) {
  ApiException e(503, 0, "Service Unavailable");
  EXPECT_NE(std::string(e.what()).find("status=503"), std::string::npos);
  EXPECT_EQ(std::string(e.what()).find("code="), std::string::npos);
}

TEST(ApiExceptionTest, IsBaseOfNovadaException) {
  ApiException e(500, 0, "boom");
  EXPECT_NO_THROW({
    const NovadaException& base = e;
    (void)base;
  });
}

TEST(IsAuthErrorTest, DetectsAuthStatuses) {
  EXPECT_TRUE(novada::is_auth_error(ApiException(401, 0, "unauthorized")));
  EXPECT_TRUE(novada::is_auth_error(ApiException(403, 0, "forbidden")));
  EXPECT_FALSE(novada::is_auth_error(ApiException(429, 0, "too many")));
  EXPECT_FALSE(novada::is_auth_error(ApiException(200, 5, "business error")));
}

TEST(IsRateLimitedTest, DetectsRateLimitStatus) {
  EXPECT_TRUE(novada::is_rate_limited(ApiException(429, 0, "slow down")));
  EXPECT_FALSE(novada::is_rate_limited(ApiException(401, 0, "unauthorized")));
}

TEST(ThrowApiExceptionTest, SelectsAuthExceptionFor401And403) {
  EXPECT_THROW({ novada::detail::throw_api_exception(401, 0, "no"); }, AuthException);
  EXPECT_THROW({ novada::detail::throw_api_exception(403, 0, "no"); }, AuthException);
}

TEST(ThrowApiExceptionTest, SelectsRateLimitExceptionFor429) {
  EXPECT_THROW({ novada::detail::throw_api_exception(429, 0, "slow down"); }, RateLimitException);
}

TEST(ThrowApiExceptionTest, SelectsPlainApiExceptionOtherwise) {
  try {
    novada::detail::throw_api_exception(500, 0, "boom");
    FAIL() << "expected ApiException";
  } catch (const AuthException&) {
    FAIL() << "should not be an AuthException";
  } catch (const RateLimitException&) {
    FAIL() << "should not be a RateLimitException";
  } catch (const ApiException& e) {
    EXPECT_EQ(e.http_status(), 500);
  }
}

TEST(ThrowApiExceptionTest, AuthExceptionIsCatchableAsApiException) {
  EXPECT_THROW({ novada::detail::throw_api_exception(401, 0, "no"); }, ApiException);
}

TEST(ValidationExceptionTest, ReportsMethodAndFields) {
  ValidationException e("Whitelist.List", {"product", "ip"});
  EXPECT_EQ(e.method(), "Whitelist.List");
  ASSERT_EQ(e.fields().size(), 2u);
  EXPECT_EQ(e.fields()[0], "product");
  EXPECT_EQ(e.fields()[1], "ip");
  EXPECT_NE(std::string(e.what()).find("Whitelist.List"), std::string::npos);
  EXPECT_NE(std::string(e.what()).find("product, ip"), std::string::npos);
}

TEST(ValidationExceptionTest, IsBaseOfNovadaException) {
  ValidationException e("Method", {"field"});
  EXPECT_NO_THROW({
    const NovadaException& base = e;
    (void)base;
  });
}

}  // namespace
