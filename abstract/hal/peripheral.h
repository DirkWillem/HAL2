#pragma once

#include <concepts>

namespace hal {

template <typename PId>
concept PeripheralId = std::equality_comparable<PId>;

template <typename P>
concept Peripheral = requires {
  { P::Used } -> std::convertible_to<bool>;
} && (!P::Used || requires {
                       { P::instance() } -> std::convertible_to<P&>;
                     });

template<typename Impl>
struct UnusedPeripheral {
  static constexpr auto Used = false;

  [[nodiscard]] static auto& instance() noexcept {
    static Impl inst{};
    return inst;
  }
};

class UsedPeripheral {
 public:
  static constexpr auto Used = true;

  UsedPeripheral()                                 = default;
  UsedPeripheral(const UsedPeripheral&)            = delete;
  UsedPeripheral(UsedPeripheral&&)                 = delete;
  UsedPeripheral& operator=(const UsedPeripheral&) = delete;
  UsedPeripheral& operator=(UsedPeripheral&&)      = delete;
};

template <Peripheral P>
[[nodiscard]] consteval bool IsPeripheralInUse() {
  return P::Used;
}

}   // namespace hal