#include <concepts>

#include <gtest/gtest.h>

#include <constexpr_tools/static_string.h>

using std::operator""sv;

template <std::size_t N = 128>
consteval auto
CreateStaticString(std::invocable<ct::StaticString<N>&> auto create) noexcept {
  ct::StaticString<N> str{};
  create(str);
  return str;
}

TEST(StaticString, EmptyString) {
  constexpr auto Str = CreateStaticString([](auto& str) {});

  ASSERT_EQ(Str.view(), ""sv);
}

TEST(StaticString, AppendChar) {
  constexpr auto Str1 =
      CreateStaticString([](auto& str) { str.AppendChar('A'); });
  constexpr auto Str2 = CreateStaticString([](auto& str) {
    str.AppendChar('A');
    str.AppendChar('B');
    str.AppendChar('C');
  });

  ASSERT_EQ(Str1.view(), "A"sv);
  ASSERT_EQ(Str2.view(), "ABC"sv);
}

TEST(StaticString, AppendCharReturnsTrueWhenSuccess) {
  ct::StaticString<8> str{};
  ASSERT_TRUE(str.AppendChar('A'));
}

TEST(StaticString, AppendCharReturnsFalseWhenFailure) {
  ct::StaticString<1> str{};
  str.AppendChar('A');
  ASSERT_FALSE(str.AppendChar('B'));
}

TEST(StaticString, AppendPositiveInteger) {
  constexpr auto str_0 = CreateStaticString([](auto& str) { str.Append(0); });
  constexpr auto str_5 = CreateStaticString([](auto& str) { str.Append(5); });
  constexpr auto str_123 =
      CreateStaticString([](auto& str) { str.Append(123); });

  ASSERT_EQ(str_0.view(), "0"sv);
  ASSERT_EQ(str_5.view(), "5"sv);
  ASSERT_EQ(str_123.view(), "123"sv);
}

TEST(StaticString, AppendNegativeInteger) {
  constexpr auto str_m5 = CreateStaticString([](auto& str) { str.Append(-5); });
  constexpr auto str_m123 =
      CreateStaticString([](auto& str) { str.Append(-123); });

  ASSERT_EQ(str_m5.view(), "-5"sv);
  ASSERT_EQ(str_m123.view(), "-123"sv);
}

TEST(StaticString, AppendString) {
  constexpr auto one_str =
      CreateStaticString([](auto& str) { str.Append("Hello!"sv); });
  constexpr auto mult_strs = CreateStaticString([](auto& str) {
    str.Append("Hello");
    str.Append(", ");
    str.Append("World!");
  });

  ASSERT_EQ(one_str.view(), "Hello!");
  ASSERT_EQ(mult_strs.view(), "Hello, World!");
}
