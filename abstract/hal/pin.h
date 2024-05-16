#pragma once

#include <array>
#include <cassert>
#include <concepts>
#include <optional>

namespace hal {

enum class PinPull {
  NoPull,
  PullUp,
  PullDown,
};

enum class PinMode { PushPull, OpenDrain };

enum class PinDirection { Input, Output, Analog };

template <typename P>
concept PinId = std::equality_comparable<P>;

template <typename Impl, typename PId>
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

template <PinId PId, std::equality_comparable Periph>
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

template <PinId PId, std::equality_comparable Periph, std::size_t N>
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

template <PinId PId, std::equality_comparable Periph, std::size_t N>
[[nodiscard]] consteval AFMapping<PId, Periph>
GetPinAFMapping(const std::array<AFMapping<PId, Periph>, N>& mappings,
                PId                                          pin) noexcept {
  const auto opt = FindPinAFMapping(mappings, pin);
  assert(("Mapping must exist", opt.has_value()));
  return *opt;
}

template <typename Impl, typename Pin>
/**
 * Concept for implementation types of General-Purpose Inputs
 * @tparam Pin Pin type
 * @tparam Impl Implementation type
 */
concept Gpi = PinId<Pin> && requires(const Impl& cgpi) {
  Impl{std::declval<Pin>()};
  Impl{std::declval<Pin>(), std::declval<hal::PinPull>()};
  Impl{std::declval<Pin>(), std::declval<hal::PinPull>(),
       std::declval<hal::PinMode>()};

  { cgpi.Read() } -> std::convertible_to<bool>;
};

template <typename Impl>
/**
 * Concept for implementation types of General-Purpose Outputs
 */
concept Gpo = requires(Impl gpo) {
  gpo.Write(std::declval<bool>());
  gpo.Toggle();
};

template <typename Impl, typename Pin>
/**
 * Concept for implementation types of General-Purpose Outputs that can be
 * constructed from a pin ID
 * @tparam Pin Pin type
 * @tparam Impl Implementation type
 */
concept ConstructibleGpo = PinId<Pin> && Gpo<Impl> && requires(Impl gpo) {
  Impl{std::declval<Pin>()};
  Impl{std::declval<Pin>(), std::declval<hal::PinPull>()};
  Impl{std::declval<Pin>(), std::declval<hal::PinPull>(),
       std::declval<hal::PinMode>()};
};

}   // namespace hal