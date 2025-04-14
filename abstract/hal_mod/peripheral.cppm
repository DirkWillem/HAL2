module;

#include <concepts>

export module hal.abstract:peripheral;

namespace hal {

export template <typename PId>
concept PeripheralId = std::equality_comparable<PId>;

export template <typename P>
concept Peripheral = requires {
  { P::Used } -> std::convertible_to<bool>;
} && (!P::Used || requires {
                       { P::instance() } -> std::convertible_to<P&>;
                     });

export template <typename Impl>
struct UnusedPeripheral {
  static constexpr auto Used = false;

  [[nodiscard]] static auto& instance() noexcept {
    static Impl inst{};
    return inst;
  }
};

export class UsedPeripheral {
 public:
  static constexpr auto Used = true;

  UsedPeripheral()                                 = default;
  UsedPeripheral(const UsedPeripheral&)            = delete;
  UsedPeripheral(UsedPeripheral&&)                 = delete;
  UsedPeripheral& operator=(const UsedPeripheral&) = delete;
  UsedPeripheral& operator=(UsedPeripheral&&)      = delete;
};

export template <Peripheral P>
[[nodiscard]] consteval bool IsPeripheralInUse() {
  return P::Used;
}

}   // namespace hal