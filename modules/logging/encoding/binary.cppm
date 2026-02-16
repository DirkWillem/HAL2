module;

#include <bit>
#include <cstdint>
#include <cstring>
#include <span>

export module logging:encoding.binary;

import hstd;

import logging.abstract;

namespace logging::encoding {

inline constexpr std::byte StartByte{'L'};

template <typename M>
struct MessageHelper;

template <auto Msg>
struct MessageHelper<Message<Msg>> {
  static constexpr std::size_t NumArgs  = 0;
  static constexpr std::size_t DataSize = 0;
};

template <auto Msg, typename... Args>
  requires(sizeof...(Args) > 0)
struct MessageHelper<Message<Msg, Args...>> {
  static constexpr std::size_t NumArgs  = sizeof...(Args);
  static constexpr std::size_t DataSize = (... + sizeof(Args));
};

export class Binary {
 public:
  template <concepts::Module Mod, concepts::Message Msg>
  static std::span<const std::byte> Encode(uint32_t timestamp, Level level,
                                           const Msg&           message,
                                           std::span<std::byte> into) {
    using H = MessageHelper<Msg>;

    constexpr std::size_t HeaderSize = sizeof(std::byte)      // Start byte
                                       + sizeof(uint32_t)     // Timestamp
                                       + sizeof(uint8_t)      // Level
                                       + sizeof(uint16_t)     // Module ID
                                       + sizeof(std::byte)    // Message ID
                                       + sizeof(std::byte);   // Payload length
    constexpr std::size_t FooterSize = sizeof(uint16_t);      // CRC

    // Encode header
    into[0] = StartByte;
    hstd::IntoByteArray(into.subspan(1), timestamp);
    into[5]  = static_cast<std::byte>(level);
    hstd::IntoByteArray(into.subspan(6), static_cast<uint16_t>(Mod::Id));
    into[8]  = static_cast<std::byte>(Mod::template MessageIndex<Msg>() + 1);
    into[9]  = static_cast<std::byte>(H::DataSize);
    auto dst = into.subspan(HeaderSize);

    // Encode arguments
    [&dst, &message]<std::size_t... Is>(std::index_sequence<Is...>) {
      (..., [&dst, &message]<std::size_t I>(hstd::ValueMarker<I>) {
        const auto& arg = std::get<I>(message.args);
        std::memcpy(dst.data(), &arg, sizeof(arg));
        dst = dst.subspan(sizeof(arg));
      }(hstd::ValueMarker<Is>()));
    }(std::make_index_sequence<H::NumArgs>());

    // Calculate CRC
    const auto crc_data = into.first(H::DataSize + HeaderSize);
    const auto crc      = hstd::Crc16(crc_data);
    hstd::IntoByteArray(dst, crc);

    return into.first(H::DataSize + HeaderSize + FooterSize);
  }
};

}   // namespace logging::encoding