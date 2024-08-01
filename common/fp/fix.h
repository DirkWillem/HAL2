#pragma once

#include <algorithm>
#include <type_traits>

namespace fp {

namespace detail {

template <bool Signed, unsigned Bits>
struct IntN;

template <bool Signed, unsigned Bits>
  requires(Bits <= 8)
struct IntN<Signed, Bits> {
  using T = std::conditional_t<Signed, int8_t, uint8_t>;
};

template <bool Signed, unsigned Bits>
  requires(Bits > 8 && Bits <= 16)
struct IntN<Signed, Bits> {
  using T = std::conditional_t<Signed, int16_t, uint16_t>;
};

template <bool Signed, unsigned Bits>
  requires(Bits > 16 && Bits <= 32)
struct IntN<Signed, Bits> {
  using T = std::conditional_t<Signed, int32_t, uint32_t>;
};

template <bool Signed, unsigned Bits>
  requires(Bits > 32 && Bits <= 64)
struct IntN<Signed, Bits> {
  using T = std::conditional_t<Signed, int64_t, uint64_t>;
};

template <bool Signed, unsigned Bits>
using IntN_t = IntN<Signed, Bits>::T;

}   // namespace detail

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

template <bool S, unsigned W, unsigned F, int Q = -static_cast<int>(F)>
  requires((S && W >= (F + 1)) || (!S && W >= F)) && (W >= F)
class Fix {
 public:
  friend class Fix;

  template <bool Sl, unsigned Wl, unsigned Fl, int Ql, bool Sr, unsigned Wr,
            unsigned Fr, int Qr>
  friend constexpr auto operator+(Fix<Sl, Wl, Fl, Ql> lhs,
                                  Fix<Sr, Wr, Fr, Qr> rhs) noexcept;

  template <bool Sl, unsigned Wl, unsigned Fl, int Ql, bool Sr, unsigned Wr,
            unsigned Fr, int Qr>
  friend constexpr auto operator-(Fix<Sl, Wl, Fl, Ql> lhs,
                                  Fix<Sr, Wr, Fr, Qr> rhs) noexcept;

  template <bool Sl, unsigned Wl, unsigned Fl, int Ql, bool Sr, unsigned Wr,
            unsigned Fr, int Qr>
  friend constexpr auto operator*(Fix<Sl, Wl, Fl, Ql> lhs,
                                  Fix<Sr, Wr, Fr, Qr> rhs) noexcept;

  template <bool Sl, unsigned Wl, unsigned Fl, int Ql, bool Sr, unsigned Wr,
            unsigned Fr, int Qr>
  friend constexpr auto operator/(Fix<Sl, Wl, Fl, Ql> lhs,
                                  Fix<Sr, Wr, Fr, Qr> rhs) noexcept;

  using Storage                  = detail::IntN_t<S, W>;
  static constexpr auto WordBits = W;
  static constexpr auto FracBits = F;
  static constexpr auto Exponent = Q;

  static constexpr Storage Scale = static_cast<Storage>(0b1)
                                   << static_cast<unsigned>(Q > 0 ? Q : -Q);
  static constexpr float Epsilon = Q > 0 ? static_cast<float>(Scale)
                                         : 1.F / static_cast<float>(Scale);

  Fix() = default;

  explicit constexpr Fix(Storage val) noexcept
      : val{val} {}

  [[nodiscard]] constexpr explicit operator float() const noexcept {
    constexpr auto Mul =
        Q < 0 ? 1.F / static_cast<float>(1U << static_cast<unsigned>(-Q))
              : static_cast<float>(1U < static_cast<unsigned>(Q));

    return static_cast<float>(val) * Mul;
  }

  [[nodiscard]] constexpr auto Round() const noexcept {
    if constexpr (Q > 0) {
      using Result = detail::IntN_t<S, W + Q>;
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

  template <unsigned Wn, unsigned Fn, int Qn = -static_cast<int>(Fn)>
  [[nodiscard]] constexpr Fix<S, Wn, Fn, Qn> As() const noexcept {
    using StorageOut = detail::IntN_t<S, Wn>;

    if constexpr (Wn < W) {
      Storage        val_os{val};
      constexpr auto Msk = (static_cast<Storage>(1U) << Wn) - 1;
      if constexpr (Qn > Q) {
        val_os >>= Qn - Q;
      } else {
        val_os <<= Q - Qn;
      }

      return Fix<S, Wn, Fn, Qn>{static_cast<StorageOut>(val_os & Msk)};
    } else {
      auto val_ns = static_cast<StorageOut>(val);
      if constexpr (Qn > Q) {
        val_ns >>= Qn - Q;
      } else {
        val_ns <<= Q - Qn;
      }

      return Fix<S, Wn, Fn, Qn>{val_ns};
    }
  }

  [[nodiscard]] auto Reciprocal() const noexcept {
    constexpr auto IntegerBits = W - F;
    constexpr auto Fo          = std::max(IntegerBits, F);
    constexpr auto Wo          = 2 * Fo;
    constexpr auto DeltaQ      = Q + static_cast<int>(F);
    constexpr auto Qo          = -static_cast<int>(Fo) - DeltaQ;
    using StorageOut           = detail::IntN_t<S, Wo>;

    // Shift value to [0.5, 1] range
    constexpr auto OneRaw  = 1 << Fo;
    constexpr auto HalfRaw = 1 << (Fo - 1U);

    auto ar    = static_cast<StorageOut>(val << (Fo - F));
    int  shift = 0;

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
    const auto a   = Fix<S, Wo, Fo, Q>{ar};
    auto       xi  = a;
    const auto Two = Fix<S, Wo, Fo, Q>{2 << Fo};

    for (int i = 0; i < 10; i++) {
      const decltype(xi) xi_new =
          (xi * (Two - (xi * a).template As<Wo, Fo, Q>()))
              .template As<Wo, Fo, Q>();

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

    return Fix<S, Wo, Fo, Qo>{result_raw};
  }

  [[nodiscard]] static constexpr auto Approximate(float v) noexcept {
    constexpr auto Mul =
        Q < 0 ? 1.F / static_cast<float>(1U << static_cast<unsigned>(-Q))
              : static_cast<float>(1U < static_cast<unsigned>(Q));

    return Fix<S, W, F, Q>{::fp::Round<Storage>(v / Mul)};
  }

  [[nodiscard]] static constexpr auto FromInt(Storage v) noexcept {
    if (Q > 0) {
      return Fix<S, W, F, Q>{
          static_cast<Storage>(v >> static_cast<unsigned>(Q))};
    } else {
      return Fix<S, W, F, Q>{
          static_cast<Storage>(v << static_cast<unsigned>(-Q))};
    }
  }

  [[nodiscard]] constexpr Storage raw() const noexcept { return val; }

 private:
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

  using Storage = detail::IntN_t<So, Wo>;

  return Fix<So, Wo, Fo, Qo>{static_cast<Storage>(
      static_cast<Storage>(lhs.template As<Wo, Fo, Qo>().val)
      + static_cast<Storage>(rhs.template As<Wo, Fo, Qo>().val))};
}

template <bool Sl, unsigned Wl, unsigned Fl, int Ql, bool Sr, unsigned Wr,
          unsigned Fr, int Qr>
constexpr auto operator-(Fix<Sl, Wl, Fl, Ql> lhs,
                         Fix<Sr, Wr, Fr, Qr> rhs) noexcept {
  constexpr auto So = Sl || Sr;
  constexpr auto Qo = std::min(Ql, Qr);
  constexpr auto Wo = std::max(Wl, Wr);
  constexpr auto Fo = std::max(Fl, Fr);

  using Storage = detail::IntN_t<So, Wo>;

  return Fix<So, Wo, Fo, Qo>{static_cast<Storage>(
      static_cast<Storage>(lhs.template As<Wo, Fo, Qo>().val)
      - static_cast<Storage>(rhs.template As<Wo, Fo, Qo>().val))};
}

template <bool Sl, unsigned Wl, unsigned Fl, int Ql, bool Sr, unsigned Wr,
          unsigned Fr, int Qr>
constexpr auto operator*(Fix<Sl, Wl, Fl, Ql> lhs,
                         Fix<Sr, Wr, Fr, Qr> rhs) noexcept {
  constexpr auto So = Sl || Sr;
  constexpr auto Wo = Wl + Wr;
  constexpr auto Fo = Fl + Fr;
  constexpr auto Qo = Ql + Qr;

  using Storage = detail::IntN_t<So, Wo>;

  return Fix<So, Wo, Fo, Qo>{static_cast<Storage>(
      static_cast<Storage>(lhs.val) * static_cast<Storage>(rhs.val))};
}

template <bool Sl, unsigned Wl, unsigned Fl, int Ql, bool Sr, unsigned Wr,
          unsigned Fr, int Qr>
constexpr auto operator/(Fix<Sl, Wl, Fl, Ql> lhs,
                         Fix<Sr, Wr, Fr, Qr> rhs) noexcept {
  return (lhs * rhs.Reciprocal());
}

template <unsigned M, unsigned N>
using UQ = Fix<false, M + N, N>;

template <typename T>
struct is_fixed_point : std::false_type {};

template <bool S, unsigned W, unsigned F, int Q>
struct is_fixed_point<Fix<S, W, F, Q>> : std::true_type {};

template <typename T>
inline constexpr bool is_fixed_point_v = is_fixed_point<T>::value;

template <typename T>
concept FixedPointType = is_fixed_point_v<T>;

}   // namespace fp

[[nodiscard]] constexpr fp::UQ<1, 15>
operator""_uq1_15(unsigned long long int v) noexcept {
  return fp::UQ<1, 15>::FromInt(static_cast<uint16_t>(v));
}
