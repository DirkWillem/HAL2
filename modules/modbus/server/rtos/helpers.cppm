module;

#include <chrono>
#include <cstdint>
#include <expected>
#include <memory>
#include <span>
#include <tuple>

export module modbus.server.rtos:helpers;

import hstd;

import rtos.concepts;

import modbus.server;

namespace modbus::server::rtos {

struct NotifyingRegisterStorageSettings {
  bool external_event_group =
      false;   //!< Whether the event group is a reference or an internally
               //!< contained value
};

export template <::rtos::concepts::Rtos             OS, typename S,
                 NotifyingRegisterStorageSettings Opts = {}>
  requires PlainMemoryRegisterStorage<S, S>
class NotifyingRegisterStorage {
  using EG = typename OS::EventGroup;

 public:
  static constexpr std::size_t Size = sizeof(S);

  explicit NotifyingRegisterStorage(std::tuple<S, EG*, uint32_t> args) noexcept
    requires(Opts.external_event_group)
      : storage{std::get<0>(args)}
      , eg{*std::get<1>(args)}
      , eb{std::get<2>(args)} {}

  explicit NotifyingRegisterStorage(std::tuple<EG*, uint32_t> args) noexcept
    requires(Opts.external_event_group)
      : storage{}
      , eg{*std::get<0>(args)}
      , eb{std::get<1>(args)} {}

  explicit NotifyingRegisterStorage() noexcept
    requires(!Opts.external_event_group)
      : storage{}
      , eg{}
      , eb{0b1UL} {}

  auto Read(std::size_t offset, std::size_t size) const noexcept {
    return ::modbus::server::ReadRegister(storage, offset, size);
  }

  auto Write(std::span<const std::byte> data, std::size_t offset,
             std::size_t size) noexcept {
    const auto result =
        ::modbus::server::WriteRegister(storage, data, offset, size);
    if (result.has_value()) {
      eg.SetBits(eb);
    }

    return result;
  }
  static void SwapEndianness(std::span<std::byte> data, std::size_t offset,
                             std::size_t size) noexcept {
    server::SwapRegisterEndianness<S>(data, offset, size);
  }

  EG&                event_group() & noexcept { return eg; }
  constexpr auto     event_bit() const noexcept { return eb; }
  constexpr const S& value() const& noexcept { return storage; }

 private:
  S storage;

  std::conditional_t<Opts.external_event_group, EG&, EG> eg;
  uint32_t                                               eb;
};

export template <::rtos::concepts::Rtos OS, std::size_t BitCount,
                 std::size_t FirstBit = 0>
class EventGroupBitStorage {
  using EG = typename OS::EventGroup;

 public:
  static constexpr std::size_t MaxBitCount = BitCount;
  using MemType                            = uint32_t;

  explicit EventGroupBitStorage(uint32_t initial_value = 0)
      : eg{} {
    if (initial_value != 0) {
      eg.SetBits(initial_value);
    }
  }

  std::expected<uint32_t, ExceptionCode> Read(uint32_t msk) const noexcept {
    return eg.ReadBits() & msk;
  }

  std::expected<uint32_t, ExceptionCode> Write(uint32_t msk,
                                               uint32_t value) noexcept {
    return eg.SetBits(value & msk);
  }

  auto&       event_group() & noexcept { return eg; }
  const auto& event_group() const& noexcept { return eg; }

 private:
  EG eg;
};

}   // namespace modbus::server::freertos