#pragma once

#include <array>
#include <concepts>

#include "vector.h"

namespace math {

struct RawElementsMarker {};

template <std::size_t M, std::size_t N, std::floating_point F>
class Mat {
 public:
  constexpr Mat() noexcept                      = default;
  constexpr Mat(const Mat&) noexcept            = default;
  constexpr Mat(Mat&&) noexcept                 = default;
  constexpr Mat& operator=(const Mat&) noexcept = default;
  constexpr Mat& operator=(Mat&&) noexcept      = default;
  constexpr ~Mat() noexcept                     = default;

  constexpr Mat(std::array<F, M * N> elements,
                [[maybe_unused]] RawElementsMarker) noexcept
      : elements{elements} {}

  explicit constexpr Mat(std::array<std::array<F, M>, N> data) noexcept
      : elements{} {
    for (std::size_t i = 0; i < M; i++) {
      for (std::size_t j = 0; j < N; j++) {
        elements[SubToInd(i, j)] = data[j][i];
      }
    }
  }

  static constexpr Mat<M, N, F> Identity() noexcept
    requires(M == N)
  {
    std::array<F, M * N> data{0.F};
    for (auto i = 0; i < M; i++) {
      data[SubToInd(i, i)] = 1.F;
    }

    return {data, RawElementsMarker{}};
  }

  [[nodiscard]] constexpr F At(std::size_t i, std::size_t j) const noexcept {
    return elements[SubToInd(i, j)];
  }

  constexpr Vec<N, F> operator*(const Vec<M, F>& rhs) const noexcept {
    Vec<N, F> result{};

    for (std::size_t i = 0; i < M; i++) {
      for (std::size_t j = 0; j < N; j++) {
        result.vals[j] += rhs.vals[i] * At(i, j);
      }
    }

    return result;
  }

  [[nodiscard]] constexpr Mat<M, N, F>
  operator+(const Mat<M, N, F>& rhs) const noexcept {
    Mat<M, N, F> result{};

    if consteval {
      DefaultAdd(rhs, result);
    } else {
      if (!CmsisDspAdd(*this, rhs, result)) {
        DefaultAdd(rhs, result);
      }
    }

    return result;
  }

  constexpr Mat<M, N, F>& operator+=(const Mat<M, N, F>& rhs) noexcept {
    if consteval {
      DefaultAddInPlace(rhs);
    } else {
      Mat<M, N, F> new_this{};
      if (CmsisDspAdd(*this, rhs, new_this)) {
        *this = new_this;
      } else {
        DefaultAddInPlace(rhs);
      }
    }

    return *this;
  }

  [[nodiscard]] constexpr Mat<M, N, F>
  operator-(const Mat<M, N, F>& rhs) const noexcept {
    Mat<M, N, F> result{};

    if consteval {
      DefaultSub(rhs, result);
    } else {
      if (!CmsisDspSub(*this, rhs, result)) {
        DefaultSub(rhs, result);
      }
    }

    return result;
  }

  constexpr Mat<M, N, F>& operator-=(const Mat<M, N, F>& rhs) noexcept {
    if consteval {
      DefaultSubInPlace(rhs);
    } else {
      Mat<M, N, F> new_this{};
      if (CmsisDspSub(*this, rhs, new_this)) {
        *this = new_this;
      } else {
        DefaultSubInPlace(rhs);
      }
    }

    return *this;
  }

  template <std::size_t N2>
  [[nodiscard]] constexpr Mat<M, N2, F>
  operator*(const Mat<N, N2, F>& rhs) const noexcept {
    Mat<M, N, F> result{};

    if consteval {
      DefaultMul(rhs, result);
    } else {
      if (!CmsisDspMul(*this, rhs, result)) {
        DefaultMul(rhs, result);
      }
    }

    return result;
  }

  constexpr Mat<M, N, F>& operator*=(const Mat<M, N, F>& rhs) noexcept
    requires(M == N)
  {
    if consteval {
      DefaultMulInPlace(rhs);
    } else {
      Mat<M, N, F> new_this{};
      if (CmsisDspMul(*this, rhs, new_this)) {
        *this = new_this;
      } else {
        DefaultSubInPlace(rhs);
      }
    }

    return *this;
  }

 private:
#ifdef HAS_CMSIS_DSP
  auto AsCmsisDspMatrix() noexcept {
    if constexpr (std::is_same_v<F, float>) {
      return arm_matrix_instance_f32{
          .numCols = M, .numRows = N, .pData = elements.data()};
    } else if constexpr (std::is_same_v<F, double>) {
      return arm_matrix_instance_f64{
          .numCols = M, .numRows = N, .pData = elements.data()};
    } else {
      // Unsupported numeric type for using matrix as CMSIS-DSP matrix
      std::unreachable();
    }
  }

  auto AsCmsisDspMatrix() const noexcept {
    if constexpr (std::is_same_v<F, float>) {
      using R = const arm_matrix_instance_f32;
      return R{.numCols = M,
               .numRows = N,
               .pData   = const_cast<float*>(elements.data())};
    } else if constexpr (std::is_same_v<F, double>) {
      return arm_matrix_instance_f64{
          .numCols = M, .numRows = N, .pData = elements.data()};
    } else {
      // Unsupported numeric type for using matrix as CMSIS-DSP matrix
      std::unreachable();
    }
  }
#endif

  constexpr void DefaultAdd(const Mat<M, N, F>& rhs,
                            Mat<M, N, F>&       dst) const noexcept {
    for (std::size_t k = 0; k < M * N; k++) {
      dst.elements[k] = elements[k] + rhs.elements[k];
    }
  }

  constexpr void DefaultAddInPlace(const Mat<M, N, F>& rhs) noexcept {
    DefaultAdd(rhs, *this);
  }

  constexpr void DefaultSub(const Mat<M, N, F>& rhs,
                            Mat<M, N, F>&       dst) const noexcept {
    for (std::size_t k = 0; k < M * N; k++) {
      dst.elements[k] = elements[k] - rhs.elements[k];
    }
  }

  constexpr void DefaultSubInPlace(const Mat<M, N, F>& rhs) noexcept {
    DefaultSub(rhs, *this);
  }

  template <std::size_t N2>
  constexpr void DefaultMul(const Mat<N, N2, F>& rhs,
                            Mat<M, N2, F>&       dst) const noexcept {
    for (std::size_t i = 0; i < M; i++) {
      for (std::size_t j = 0; j < N2; j++) {
        for (std::size_t k = 0; k < N; k++) {
          dst.elements[SubToInd(i, j)] +=
              elements[SubToInd(i, k)] * rhs.elements[SubToInd(k, j)];
        }
      }
    }
  }

  template <std::size_t N2>
  constexpr void DefaultMulInPlace(const Mat<N, N2, F>& rhs) noexcept {
    Mat<N, N2, F> tmp{};
    DefaultMul(rhs, tmp);
    *this = tmp;
  }

  inline static bool CmsisDspAdd(const Mat<M, N, F>& lhs,
                                 const Mat<M, N, F>& rhs,
                                 Mat<M, N, F>&       dst) noexcept {
#if defined(HAS_CMSIS_DSP)
    const auto lhs_mat = lhs.AsCmsisDspMatrix();
    const auto rhs_mat = rhs.AsCmsisDspMatrix();
    auto       dst_mat = dst.AsCmsisDspMatrix();

    if constexpr (std::is_same_v<F, float>) {
      return arm_mat_add_f32(&lhs_mat, &rhs_mat, &dst_mat) == ARM_MATH_SUCCESS;
    } else {
      // arm_mat_add_* not supported for current floating point type
      std::unreachable();
    }
#else
    return false;
#endif
  }

  inline static void CmsisDspSub(const Mat<M, N, F>& lhs,
                                 const Mat<M, N, F>& rhs,
                                 Mat<M, N, F>&       dst) noexcept {
#if defined(HAS_CMSIS_DSP)
    const auto lhs_mat = lhs.AsCmsisDspMatrix();
    const auto rhs_mat = rhs.AsCmsisDspMatrix();
    auto       dst_mat = dst.AsCmsisDspMatrix();

    if constexpr (std::is_same_v<F, float>) {
      return arm_mat_sub_f32(&lhs_mat, &rhs_mat, &dst_mat) == ARM_MATH_SUCCESS;
    } else if constexpr (std::is_same_v<F, double>) {
      return arm_mat_sub_f64(&lhs_mat, &rhs_mat, &dst_mat) == ARM_MATH_SUCCESS;
    } else {
      // arm_mat_add_* not supported for current floating point type
      std::unreachable();
    }
#else
    return false;
#endif
  }

  template <std::size_t N2>
  inline static bool CmsisDspMul(const Mat<M, N, F>&  lhs,
                                 const Mat<N, N2, F>& rhs,
                                 Mat<M, N2, F>&       dst) noexcept {
#if defined(HAS_CMSIS_DSP)
    const auto lhs_mat = lhs.AsCmsisDspMatrix();
    const auto rhs_mat = rhs.AsCmsisDspMatrix();
    auto       dst_mat = dst.AsCmsisDspMatrix();

    if constexpr (std::is_same_v<F, float>) {
      return arm_mat_mult_f32(&lhs_mat, &rhs_mat, &dst_mat) == ARM_MATH_SUCCESS;
    } else if constexpr (std::is_same_v<F, double>) {
      return arm_mat_mult_f64(&lhs_mat, &rhs_mat, &dst_mat) == ARM_MATH_SUCCESS;
    } else {
      // arm_mat_sub_* not supported for current floating point type
      std::unreachable();
    }
#else
    return false;
#endif
  }

  [[nodiscard]] static constexpr std::size_t SubToInd(std::size_t i,
                                                      std::size_t j) noexcept {
    return N * i + j;
  }

  std::array<F, (M * N)> elements{};
};

}   // namespace math