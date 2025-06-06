#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <type_traits>

#include <fp/fix.h>

using namespace testing;
using std::operator""sv;

using UQ8_8   = fp::UQ<8, 8>;
using UQ16_16 = fp::UQ<16, 16>;
using UQ1_15  = fp::UQ<1, 15>;
using UQ1_31  = fp::UQ<1, 31>;

// constexpr tests
// - General properties
static_assert(std::is_same_v<UQ8_8::Storage, uint16_t>);
static_assert(std::is_same_v<UQ1_15::Storage, uint16_t>);
static_assert(std::is_same_v<UQ1_31::Storage, uint32_t>);
static_assert(UQ8_8::Scale == 256);

// - Descriptions
static_assert(UQ8_8::Describe().view() == "ufix(16, -8)"sv);
static_assert(UQ1_31::Describe().view() == "ufix(32, -31)"sv);

// - Operators
static_assert(std::is_same_v<
              decltype(std::declval<UQ8_8>() + std::declval<UQ8_8>()), UQ8_8>);
static_assert(
    std::is_same_v<decltype(std::declval<UQ1_15>() + std::declval<UQ1_31>()),
                   UQ1_31>);

static_assert(std::is_same_v<
              decltype(std::declval<UQ8_8>() - std::declval<UQ8_8>()), UQ8_8>);
static_assert(
    std::is_same_v<decltype(std::declval<UQ1_15>() - std::declval<UQ1_31>()),
                   UQ1_31>);

static_assert(
    std::is_same_v<decltype(std::declval<UQ8_8>() * std::declval<UQ8_8>()),
                   fp::UQ<8, 8>>);
static_assert(
    std::is_same_v<decltype(std::declval<UQ1_15>() * std::declval<UQ1_31>()),
                   fp::UQ<1, 31>>);

static_assert(
    std::is_same_v<decltype(std::declval<UQ8_8>() / std::declval<UQ8_8>()),
                   fp::UQ<8, 8>>);

TEST(FP_UQ, FromFloat) {
  ASSERT_EQ(UQ8_8::Approximate(1.F).raw(), UQ8_8::Scale);
  ASSERT_EQ(UQ8_8::Approximate(0.25F).raw(), UQ8_8::Scale / 4);
  ASSERT_THAT(static_cast<float>(UQ1_15::Approximate(1.F / 3.F)),
              FloatNear(1.F / 3.F, UQ1_15::Epsilon));

  ASSERT_EQ(UQ8_8::Approximate(4.5F).raw(),
            UQ8_8::Scale / 2 + UQ8_8::Scale * 4);
}

TEST(FP_UQ, AdditionOfSameType) {
  const auto a = UQ1_15::Approximate(0.25F);
  const auto b = UQ1_15::Approximate(0.125F);

  ASSERT_EQ((a + a).raw(), UQ1_15::Scale / 2);
  ASSERT_EQ((a + b).raw(), (UQ1_15::Scale / 8) * 3);
}

TEST(FP_UQ, AdditionOfDifferentTypes) {
  const auto a16 = UQ1_15::Approximate(0.25F);
  const auto a32 = UQ1_31::Approximate(0.25F);
  const auto b32 = UQ1_31::Approximate(0.125F);

  ASSERT_EQ((a16 + a32).raw(), UQ1_31::Scale / 2);
  ASSERT_EQ((a16 + b32).raw(), (UQ1_31::Scale / 8) * 3);
}

TEST(FP_UQ, AdditionOfInteger) {
  ASSERT_EQ((UQ8_8::Approximate(1.5F) + 3).raw(),
            UQ8_8::Approximate(4.5F).raw());
}

TEST(FP_UQ, SubtractionOfSameType) {
  const auto a = UQ1_15::Approximate(0.25F);
  const auto b = UQ1_15::Approximate(0.125F);

  ASSERT_EQ((a - a).raw(), 0);
  ASSERT_EQ((a - b).raw(), b.raw());
}

TEST(FP_UQ, MultiplicationOfSameType) {
  const auto a      = UQ1_31::Approximate(0.5F);
  const auto result = a * a;

  ASSERT_EQ(result.raw(), result.Scale / 4);
}

TEST(FP_UQ, MultiplicationByInteger) {
  const uint16_t period     = 2500;
  const auto     duty_cycle = UQ16_16::Approximate(0.5F);

  ASSERT_EQ((duty_cycle * period).raw(), (UQ16_16{1250}.raw()));
  ASSERT_EQ((period * duty_cycle).raw(), (UQ16_16{1250}.raw()));
}

TEST(FP_UQ, Reciprocal) {
  const auto a = UQ8_8::Approximate(4.F);

  ASSERT_EQ(a.Reciprocal().raw(), UQ8_8::Scale / 4);
}

// TEST(FP_UQ, Reciprocal) {
//   const auto a = UQ8_8::FromInt(3);
//
//   ASSERT_THAT(static_cast<float>(a.Reciprocal()),
//               FloatNear(1.F / 3.F, 2*UQ8_8::Epsilon));
// }

TEST(FP_UQ, DivisionOfSameType) {
  using OutType = decltype(std::declval<UQ16_16>() / std::declval<UQ16_16>());
  const auto a  = UQ16_16{2};
  const auto b  = UQ16_16{8};
  const auto c  = UQ16_16{3};

  ASSERT_EQ((a / b).raw(), OutType::Scale / 4);
  ASSERT_EQ((b / a).raw(), OutType::Scale * 4);

  ASSERT_THAT(static_cast<float>(a / c),
              FloatNear(2.F / 3.F, 256 * UQ16_16::Epsilon));
}

TEST(FP_UQ, Round) {
  ASSERT_EQ(UQ8_8{0.25F}.Round(), 0);
  ASSERT_EQ(UQ8_8{3.45F}.Round(), 3);
  ASSERT_EQ(UQ8_8{12.75F}.Round(), 13);

  ASSERT_EQ(UQ1_15::Approximate(1.25F).Round(), 1);
  ASSERT_EQ(UQ1_15::Approximate(1.76F).Round(), 2);
}
