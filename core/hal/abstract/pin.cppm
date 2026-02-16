module;

#include <array>
#include <concepts>
#include <optional>
#include <utility>

export module hal.abstract:pin;

import hstd;

namespace hal {

export enum class PinPull {
  NoPull,
  PullUp,
  PullDown,
};

export enum class PinMode { PushPull, OpenDrain };

export enum class PinDirection { Input, Output, Analog };

export enum class Edge { Rising, Falling, Both };

/**
 * Returns an edge from two pin states
 * @param from Before pin state
 * @param to After pin state
 * @return Edge between the two states. If from == to, behavior is undefined
 */
export consteval Edge GetEdge(bool from, bool to) noexcept {
  if (!from && to) {
    return Edge::Rising;
  }

  if (from && !to) {
    return Edge::Falling;
  }

  std::unreachable();
}

export template <typename P>
concept PinId = std::equality_comparable<P>;

export template <typename Impl, typename PId>
concept Pin = PinId<PId> && requires {
  Impl::Initialize(std::declval<PId>(), std::declval<PinDirection>());
  Impl::Initialize(std::declval<PId>(), std::declval<PinDirection>(),
                   std::declval<PinPull>());
  Impl::Initialize(std::declval<PId>(), std::declval<PinDirection>(),
                   std::declval<PinPull>(), std::declval<PinMode>());

  Impl::InitializeAlternate(std::declval<PId>(), std::declval<unsigned>());
  Impl::InitializeAlternate(std::declval<PId>(), std::declval<unsigned>(),
                            std::declval<PinPull>());
  Impl::InitializeAlternate(std::declval<PId>(), std::declval<unsigned>(),
                            std::declval<PinPull>(), std::declval<PinMode>());
};

export template <PinId PId, std::equality_comparable Periph>
/**
 * Pin alternate function mapping
 * @tparam PId Pin ID type
 * @tparam Periph Peripheral type
 */
struct AFMapping {
  PId      pin;
  Periph   peripheral;
  unsigned af;
};

export template <PinId PId, std::equality_comparable Periph, std::size_t N>
[[nodiscard]] constexpr std::optional<AFMapping<PId, Periph>>
FindPinAFMapping(const std::array<AFMapping<PId, Periph>, N>& mappings,
                 Periph periph, PId pin) noexcept {
  for (const auto& mapping : mappings) {
    if (mapping.pin == pin && mapping.peripheral == periph) {
      return mapping;
    }
  }

  return {};
}

export template <PinId PId, std::equality_comparable Periph, std::size_t N>
[[nodiscard]] consteval AFMapping<PId, Periph>
GetPinAFMapping(const std::array<AFMapping<PId, Periph>, N>& mappings,
                PId                                          pin) noexcept {
  const auto opt = FindPinAFMapping(mappings, pin);
  hstd::Assert(opt.has_value(), "Mapping must exist");
  return *opt;
}

/**
 * Concept for implementation types of General-Purpose Inputs
 * @tparam Impl Implementation type
 */
export template <typename Impl>
concept Gpi = requires(const Impl& cgpi) {
  { cgpi.Read() } -> std::convertible_to<bool>;
};

/**
 * Concept for implementation types of General-Purpose Outputs
 */
export template <typename Impl>
concept Gpo = requires(Impl gpo) {
  gpo.Write(std::declval<bool>());
  gpo.Toggle();
};

/**
 * Concept for implementation types of General-Purpose Outputs that can be
 * constructed from a pin ID
 * @tparam Pin Pin type
 * @tparam Impl Implementation type
 */
export template <typename Impl, typename Pin>
concept ConstructibleGpo = PinId<Pin> && Gpo<Impl> && requires(Impl gpo) {
  Impl{std::declval<Pin>()};
  Impl{std::declval<Pin>(), std::declval<hal::PinPull>()};
  Impl{std::declval<Pin>(), std::declval<hal::PinPull>(),
       std::declval<hal::PinMode>()};
};

/**
 * Timer alternate function mapping
 * @tparam PId Pin ID type
 * @tparam Tim Timer ID type
 */
export template <PinId PId, std::equality_comparable Tim>
struct TimAFMapping {
  PId      pin;
  Tim      tim;
  unsigned ch;
  unsigned af;
};

export template <PinId PId, std::equality_comparable Tim, std::size_t N>
[[nodiscard]] constexpr std::optional<TimAFMapping<PId, Tim>>
FindTimAFMapping(const std::array<TimAFMapping<PId, Tim>, N>& mappings, Tim tim,
                 unsigned ch, PId pin) noexcept {
  for (const auto& mapping : mappings) {
    if (mapping.pin == pin && mapping.tim == tim && mapping.ch == ch) {
      return mapping;
    }
  }

  return {};
}

}   // namespace hal