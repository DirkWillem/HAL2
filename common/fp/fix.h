#pragma once

#include <algorithm>
#include <type_traits>

#include <constexpr_tools/integer.h>
#include <constexpr_tools/static_string.h>
#include <constexpr_tools/type_helpers.h>

namespace fp {

template <std::integral T>
[[nodiscard]] constexpr T Round(float v) noexcept {
  if constexpr (std::is_signed_v<T>) {
    if (v > 0) {
      return static_cast<T>(v + 0.5F);
    } else {
      return static_cast<T>(v - 0.5F);
    }
  } else {
    if (v > 0) {
      return static_cast<T>(v + 0.5F);
    } else {
      return 0;
    }
  }
}

struct ConstructRaw {};

template <bool S, unsigned W, unsigned F, int Q = -static_cast<int>(F)>
  requires((S && W >= (F + 1)) || (!S && W >= F)) && (W >= F)
class Fix {
 public:
  static consteval ct::StaticString<32> Describe() noexcept {
    ct::StaticString<32> str{};

    if constexpr (S) {
      str.Append("sfix(");
    } else {
      str.Append("ufix(");
    }

    str.Append(W);
    str.Append(", ");
    str.Append(Q);
    str.Append(")");

    return str;
  }

  static constexpr auto Name = Describe();

  friend class Fix;

  using Storage                  = ct::IntN_t<S, W>;
  static constexpr auto WordBits = W;
  static constexpr auto FracBits = F;
  static constexpr auto IntBits  = WordBits - FracBits;
  static constexpr auto Exponent = Q;

  static constexpr float Scale   = ct::Pow2(-Q);
  static constexpr float Epsilon = ct::Pow2(Q);

  Fix() = default;

  template <std::unsigned_integral U>
  explicit constexpr Fix(U val) noexcept
      : val{IntToRaw(static_cast<Storage>(val))} {}

  template <std::signed_integral I>
  explicit constexpr Fix(I val) noexcept
      : val{IntToRaw(static_cast<Storage>(val))} {}


  template <std::floating_point FP>
  explicit constexpr Fix(FP value) noexcept
      : val{FloatingPointToRaw(value)} {}

  explicit constexpr Fix([[maybe_unused]] ConstructRaw rm, Storage val) noexcept
      : val{val} {}

  [[nodiscard]] constexpr std::strong_ordering
  operator<=>(Fix<S, W, F, Q> rhs) noexcept {
    return val <=> rhs.val;
  }

  constexpr auto& operator+=(Fix<S, W, F, Q> rhs) noexcept {
    val = (*this + rhs).template As<W, F, Q>().raw();
    return *this;
  }

  constexpr auto& operator-=(Fix<S, W, F, Q> rhs) noexcept {
    val = (*this - rhs).template As<W, F, Q>().raw();
    return *this;
  }

  constexpr auto& operator*=(Fix<S, W, F, Q> rhs) noexcept {
    val = (*this * rhs).template As<W, F, Q>().raw();
    return *this;
  }

  constexpr auto& operator/=(Fix<S, W, F, Q> rhs) noexcept {
    val = (*this / rhs).template As<W, F, Q>().raw();
    return *this;
  }

  [[nodiscard]] constexpr explicit operator float() const noexcept {
    constexpr auto Mul =
        Q < 0 ? 1.F / static_cast<float>(1U << static_cast<unsigned>(-Q))
              : static_cast<float>(1U < static_cast<unsigned>(Q));

    return static_cast<float>(val) * Mul;
  }

  [[nodiscard]] constexpr explicit operator uint8_t() const noexcept
    requires(!S && IntBits <= 8)
  {
    return static_cast<uint8_t>(IntegerPart());
  }

  [[nodiscard]] constexpr explicit operator uint16_t() const noexcept
    requires(!S && IntBits <= 16)
  {
    return static_cast<uint16_t>(IntegerPart());
  }

  [[nodiscard]] constexpr explicit operator uint32_t() const noexcept
    requires(!S && IntBits <= 32)
  {
    return static_cast<uint32_t>(IntegerPart());
  }

  [[nodiscard]] constexpr explicit operator uint64_t() const noexcept
    requires(!S && IntBits <= 64)
  {
    return static_cast<uint64_t>(IntegerPart());
  }

  [[nodiscard]] constexpr explicit operator std::size_t() const noexcept
    requires(!S && IntBits <= std::numeric_limits<std::size_t>::digits)
  {
    return static_cast<std::size_t>(IntegerPart());
  }

  [[nodiscard]] constexpr auto Round() const noexcept {
    if constexpr (Q > 0) {
      using Result = ct::IntN_t<S, W + Q>;
      return static_cast<Result>(val) << Q;
    } else {
      constexpr Storage FractionalMask =
          (static_cast<Storage>(0b1U) << static_cast<unsigned>(-Q)) - 1;
      constexpr auto Half = Scale / 2;

      const auto fractional_part = val & FractionalMask;
      const auto integer_part    = val >> static_cast<unsigned>(-Q);

      if (fractional_part >= Half) {
        return integer_part + 1;
      } else {
        return integer_part;
      }
    }
  }

  [[nodiscard]] constexpr auto IntegerPart() const noexcept {
    if constexpr (Q == -static_cast<int>(F)) {
      using Result = ct::IntN_t<S, W - F>;
      return static_cast<Result>(val >> F);
    } else {
      // Not implemented
      std::unreachable();
    }
  }

  template <unsigned Wn, unsigned Fn, int Qn = -static_cast<int>(Fn)>
    requires(Wn != W || Fn != F || Qn != Q)
  [[nodiscard]] constexpr Fix<S, Wn, Fn, Qn> As() const noexcept {
    using StorageOut = ct::IntN_t<S, Wn>;

    if constexpr (Wn < W) {
      Storage        val_os{val};
      constexpr auto Msk = (static_cast<Storage>(1U) << Wn) - 1;
      if constexpr (Qn > Q) {
        val_os >>= Qn - Q;
      } else {
        val_os <<= Q - Qn;
      }

      return Fix<S, Wn, Fn, Qn>{ConstructRaw{},
                                static_cast<StorageOut>(val_os & Msk)};
    } else {
      auto val_ns = static_cast<StorageOut>(val);
      if constexpr (Qn > Q) {
        val_ns >>= Qn - Q;
      } else {
        val_ns <<= Q - Qn;
      }

      return Fix<S, Wn, Fn, Qn>{ConstructRaw{}, val_ns};
    }
  }

  template <unsigned Wn, unsigned Fn, int Qn = -static_cast<int>(Fn)>
    requires(Wn == W && Fn == F && Qn == Q)
  [[nodiscard]] constexpr Fix<S, Wn, Fn, Qn> As() const noexcept {
    return *this;
  }

  [[nodiscard]] constexpr Fix<S, W, F, Q> Reciprocal() const noexcept {
    // Shift value to [0.5, 1] range
    constexpr auto OneRaw  = 1 << F;
    constexpr auto HalfRaw = 1 << (F - 1U);

    auto       ar     = static_cast<Storage>(val << (F - F));
    const auto negate = ar < 0;
    int        shift  = 0;

    if (negate) {
      ar = -ar;
    }

    while (ar > OneRaw || ar < HalfRaw) {
      if (ar > OneRaw) {
        shift--;
        ar >>= 1;
      } else {
        shift++;
        ar <<= 1;
      }
    }

    // Perform Newton-Raphson divison
    const auto a   = Fix<S, W, F, Q>{ConstructRaw{}, ar};
    auto       xi  = a;
    const auto Two = Fix<S, W, F, Q>{2};

    for (int i = 0; i < 10; i++) {
      const decltype(xi) xi_new =
          (xi * (Two - (xi * a).template As<W, F, Q>())).template As<W, F, Q>();

      if (xi_new.raw() == xi.raw()) {
        break;
      }

      xi = xi_new;
    }

    auto result_raw = xi.raw();
    if (shift > 0) {
      result_raw <<= static_cast<unsigned>(shift);
    } else {
      result_raw >>= static_cast<unsigned>(-shift);
    }

    if (negate) {
      result_raw = -result_raw;
    }

    return Fix<S, W, F, Q>{ConstructRaw{}, result_raw};
  }

  [[nodiscard]] static constexpr auto Approximate(float v) noexcept {
    constexpr auto Mul =
        Q < 0 ? 1.F / static_cast<float>(1U << static_cast<unsigned>(-Q))
              : static_cast<float>(1U < static_cast<unsigned>(Q));

    return Fix<S, W, F, Q>{ConstructRaw{}, ::fp::Round<Storage>(v / Mul)};
  }

  [[nodiscard]] constexpr Storage raw() const noexcept { return val; }

 private:
  [[nodiscard]] static constexpr auto IntToRaw(Storage v) noexcept {
    if constexpr (Q > 0) {
      return static_cast<Storage>(v >> static_cast<unsigned>(Q));
    } else {
      return static_cast<Storage>(v << static_cast<unsigned>(-Q));
    }
  }

  template <std::floating_point FP>
  [[nodiscard]] static constexpr auto FloatingPointToRaw(FP v) noexcept {
    constexpr auto Mul =
        Q < 0 ? 1.F / static_cast<FP>(1U << static_cast<unsigned>(-Q))
              : static_cast<FP>(1U < static_cast<unsigned>(Q));

    return ::fp::Round<Storage>(v / Mul);
  }

  Storage val{0};
};

template <bool Sl, unsigned Wl, unsigned Fl, int Ql, bool Sr, unsigned Wr,
          unsigned Fr, int Qr>
constexpr auto operator+(Fix<Sl, Wl, Fl, Ql> lhs,
                         Fix<Sr, Wr, Fr, Qr> rhs) noexcept {
  constexpr auto So = Sl || Sr;
  constexpr auto Qo = std::min(Ql, Qr);
  constexpr auto Wo = std::max(Wl, Wr);
  constexpr auto Fo = std::max(Fl, Fr);

  using Storage = ct::IntN_t<So, Wo>;

  return Fix<So, Wo, Fo, Qo>{
      ConstructRaw{},
      static_cast<Storage>(
          static_cast<Storage>(lhs.template As<Wo, Fo, Qo>().raw())
          + static_cast<Storage>(rhs.template As<Wo, Fo, Qo>().raw()))};
}

template <bool Sl, unsigned Wl, unsigned Fl, int Ql, ct::Integer R>
constexpr auto operator+(Fix<Sl, Wl, Fl, Ql> lhs, R rhs) noexcept {
  constexpr auto So = Sl || std::is_signed_v<R>;
  constexpr auto Qo = Ql;
  constexpr auto Wo = Wl;
  constexpr auto Fo = Fl;

  using To = Fix<So, Wo, Fo, Qo>;

  return lhs.template As<Wo, Fo, Qo>() + To{static_cast<To::Storage>(rhs)};
}

template <ct::Integer L, bool Sr, unsigned Wr, unsigned Fr, int Qr>
constexpr auto operator+(L lhs, Fix<Sr, Wr, Fr, Qr> rhs) noexcept {
  return rhs + lhs;
}

template <bool Sl, unsigned Wl, unsigned Fl, int Ql, bool Sr, unsigned Wr,
          unsigned Fr, int Qr>
constexpr auto operator-(Fix<Sl, Wl, Fl, Ql> lhs,
                         Fix<Sr, Wr, Fr, Qr> rhs) noexcept {
  constexpr auto So = Sl || Sr;
  constexpr auto Qo = std::min(Ql, Qr);
  constexpr auto Wo = std::max(Wl, Wr);
  constexpr auto Fo = std::max(Fl, Fr);

  using Storage = ct::IntN_t<So, Wo>;

  return Fix<So, Wo, Fo, Qo>{
      ConstructRaw{},
      static_cast<Storage>(
          static_cast<Storage>(lhs.template As<Wo, Fo, Qo>().raw())
          - static_cast<Storage>(rhs.template As<Wo, Fo, Qo>().raw()))};
}

template <bool Sl, unsigned Wl, unsigned Fl, int Ql, ct::Integer R>
constexpr auto operator-(Fix<Sl, Wl, Fl, Ql> lhs, R rhs) noexcept {
  constexpr auto So = Sl || std::is_signed_v<R>;
  constexpr auto Qo = Ql;
  constexpr auto Wo = Wl;
  constexpr auto Fo = Fl;

  using To = Fix<So, Wo, Fo, Qo>;

  return lhs.template As<Wo, Fo, Qo>() - To{static_cast<To::Storage>(rhs)};
}

template <unsigned Wl, unsigned Fl, int Ql>
constexpr auto operator-(Fix<true, Wl, Fl, Ql> rhs) noexcept {
  using Result = Fix<true, Wl, Fl, Ql>;
  return Result{ConstructRaw{},
                static_cast<typename Result::Storage>(-rhs.raw())};
}

template <ct::Integer L, bool Sr, unsigned Wr, unsigned Fr, int Qr>
constexpr auto operator*(L lhs, Fix<Sr, Wr, Fr, Qr> rhs) noexcept {
  return rhs * lhs;
}

template <bool Sl, unsigned Wl, unsigned Fl, int Ql, ct::Integer R>
constexpr auto operator*(Fix<Sl, Wl, Fl, Ql> lhs, R rhs) noexcept {
  constexpr auto So = Sl || std::is_signed_v<R>;
  constexpr auto Qo = Ql;
  constexpr auto Wo = Wl;
  constexpr auto Fo = Fl;

  using Storage = ct::IntN_t<So, Wo>;

  return Fix<So, Wo, Fo, Qo>{
      ConstructRaw{},
      static_cast<Storage>(
          static_cast<Storage>(lhs.template As<Wo, Fo, Qo>().raw()) * rhs)};
}

template <bool Sl, unsigned Wl, unsigned Fl, int Ql, bool Sr, unsigned Wr,
          unsigned Fr, int Qr>
constexpr auto operator*(Fix<Sl, Wl, Fl, Ql> lhs,
                         Fix<Sr, Wr, Fr, Qr> rhs) noexcept {
  constexpr auto So = Sl || Sr;
  constexpr auto Wo = std::max(Wl, Wr);
  constexpr auto Fo = std::max(Fl, Fr);
  constexpr auto Qo = std::min(Ql, Qr);
  constexpr auto DQ = (Ql + Qr) - Qo;

  using Storage    = ct::IntN_t<So, Wl + Wr>;
  using StorageOut = ct::IntN_t<So, Wo>;

  return Fix<So, Wo, Fo, Qo>{
      ConstructRaw{},
      static_cast<StorageOut>(
          (static_cast<Storage>(lhs.template As<Wo, Fo, Ql>().raw())
           * static_cast<Storage>(rhs.template As<Wo, Fo, Qr>().raw()))
          >> static_cast<unsigned>(-DQ))};
}

template <bool Sl, unsigned Wl, unsigned Fl, int Ql, bool Sr, unsigned Wr,
          unsigned Fr, int Qr>
constexpr auto operator/(Fix<Sl, Wl, Fl, Ql> lhs,
                         Fix<Sr, Wr, Fr, Qr> rhs) noexcept {
  return (lhs * rhs.Reciprocal());
}

template <unsigned M, unsigned N>
using UQ = Fix<false, M + N, N>;

template <unsigned M, unsigned N>
  requires(M >= 1)
using Q = Fix<true, M + N, N>;

namespace literals {

[[nodiscard]] constexpr UQ<1, 15>
operator""_uq1_15(unsigned long long int v) noexcept {
  return fp::UQ<1, 15>{static_cast<uint16_t>(v)};
}

[[nodiscard]] constexpr UQ<1, 31>
operator""_uq1_31(unsigned long long int v) noexcept {
  return fp::UQ<1, 31>{static_cast<uint32_t>(v)};
}

[[nodiscard]] constexpr Q<1, 15>
operator""_u1_15(unsigned long long int v) noexcept {
  return fp::Q<1, 15>{static_cast<int16_t>(v)};
}

[[nodiscard]] constexpr Q<1, 31>
operator""_u1_31(unsigned long long int v) noexcept {
  return fp::Q<1, 31>{static_cast<int32_t>(v)};
}

}   // namespace literals

}   // namespace fp
