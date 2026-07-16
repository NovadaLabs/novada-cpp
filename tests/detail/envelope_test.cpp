#include "novada/detail/envelope.hpp"

#include <gtest/gtest.h>

#include "novada/errors.hpp"

namespace {

struct Item {
  int id = 0;
  std::string name;
};

void from_json(const nlohmann::json& j, Item& item) {
  item.id = j.value("id", 0);
  item.name = j.value("name", std::string());
}

TEST(ParseEnvelopeTest, ParsesFields) {
  auto env = novada::detail::parse_envelope(
      200, R"({"code":0,"msg":"success","data":{"a":1},"timestamp":1732084616})");
  EXPECT_EQ(env.code, 0);
  EXPECT_EQ(env.msg, "success");
  EXPECT_EQ(env.timestamp, 1732084616);
  EXPECT_EQ(env.data.at("a").get<int>(), 1);
}

TEST(ParseEnvelopeTest, InvalidJsonThrowsApiException) {
  try {
    novada::detail::parse_envelope(200, "not json");
    FAIL() << "expected ApiException";
  } catch (const novada::ApiException& e) {
    EXPECT_EQ(e.http_status(), 200);
    EXPECT_EQ(e.code(), 0);
  }
}

TEST(DecodeEnvelopeTest, ReturnsDataOnSuccess) {
  nlohmann::json data =
      novada::detail::decode_envelope(200, R"({"code":0,"msg":"ok","data":{"x":42}})");
  EXPECT_EQ(data.at("x").get<int>(), 42);
}

TEST(DecodeEnvelopeTest, ThrowsApiExceptionOnNonZeroCode) {
  try {
    novada::detail::decode_envelope(200, R"({"code":1001,"msg":"bad product","data":null})");
    FAIL() << "expected ApiException";
  } catch (const novada::ApiException& e) {
    EXPECT_EQ(e.code(), 1001);
    EXPECT_EQ(e.message(), "bad product");
  }
}

TEST(DecodeEnvelopeTest, MapsHttp401ToAuthException) {
  EXPECT_THROW(
      { novada::detail::decode_envelope(401, R"({"code":1,"msg":"unauthorized"})"); },
      novada::AuthException);
}

TEST(UnwrapListTest, DecodesListAndTotal) {
  nlohmann::json data =
      nlohmann::json::parse(R"({"list":[{"id":1,"name":"a"},{"id":2,"name":"b"}],"total":2})");
  std::vector<Item> items;
  int total = 0;
  novada::detail::unwrap_list(data, items, total);
  ASSERT_EQ(items.size(), 2u);
  EXPECT_EQ(items[0].id, 1);
  EXPECT_EQ(items[0].name, "a");
  EXPECT_EQ(items[1].id, 2);
  EXPECT_EQ(total, 2);
}

TEST(UnwrapListTest, HandlesMissingListAsEmpty) {
  nlohmann::json data = nlohmann::json::parse(R"({"total":0})");
  std::vector<Item> items;
  int total = -1;
  novada::detail::unwrap_list(data, items, total);
  EXPECT_TRUE(items.empty());
  EXPECT_EQ(total, 0);
}

}  // namespace
