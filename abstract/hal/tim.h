#pragma once

#include <array>
#include <concepts>
#include <cstdint>
#include <span>

namespace hal {

template <typename Impl>
concept Tim = requires(Impl& impl) {
  impl.Start();
  impl.Stop();
};

template <typename Impl, std::size_t N>
concept BurstDmaPwmChannel = requires(Impl& impl) {
  impl.SetCompares(std::declval<std::array<uint16_t, N>>());
  {
    impl.SetDmaData(std::declval<const std::span<uint16_t>>())
  } -> std::convertible_to<bool>;

  impl.Enable();
  impl.Disable();
};

}   // namespace hal