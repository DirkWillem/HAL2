module;

#include <cstdint>
#include <expected>
#include <memory>
#include <span>
#include <tuple>

export module modbus.server.freertos:helpers;

import hstd;

import rtos.freertos;

import modbus.server;

namespace modbus::server::freertos {

export template <typename S>
  requires PlainMemoryRegisterStorage<S, S>
class NotifyingRegisterStorage {
 public:
  static constexpr std::size_t Size = sizeof(S);

  explicit NotifyingRegisterStorage(
      std::tuple<S, ::rtos::EventGroup*, uint32_t> args) noexcept
      : storage{std::get<0>(args)}
      , event_group{*std::get<1>(args)}
      , event_bit{std::get<2>(args)} {}

  explicit NotifyingRegisterStorage(
      std::tuple<::rtos::EventGroup*, uint32_t> args) noexcept
      : storage{}
      , event_group{*std::get<0>(args)}
      , event_bit{std::get<1>(args)} {}

  auto Read(std::size_t offset, std::size_t size) const noexcept {
    return ::modbus::server::ReadRegister(storage, offset, size);
  }

  auto Write(std::span<const std::byte> data, std::size_t offset,
             std::size_t size) noexcept {
    const auto result =
        ::modbus::server::WriteRegister(storage, data, offset, size);
    if (result.has_value()) {
      event_group.SetBits(event_bit);
    }

    return result;
  }

  static void SwapEndianness(std::span<std::byte> data, std::size_t offset,
                             std::size_t size) noexcept {
    server::SwapRegisterEndianness<S>(data, offset, size);
  }

  constexpr const S& value() const& noexcept { return storage; }

 private:
  S storage;

  ::rtos::EventGroup& event_group;
  uint32_t            event_bit;
};

export template <std::size_t BitCount, std::size_t FirstBit = 0>
class EventGroupBitStorage {
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
  rtos::EventGroup eg;
};

}   // namespace modbus::server::freertos