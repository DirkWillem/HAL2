module;

#include <string>
#include <string_view>
#include <vector>

export module modbus.server.spec.gen:bits;

import hstd;

import modbus.server.spec;

namespace modbus::server::spec::gen {

/**
 * Information on a single bit
 */
export struct BitInfo {
  uint16_t    address;   //!< Bit address
  std::string name;      //!< Bit name
  BitAccess   access;    //!< Bit access
};

/**
 * Information on one or more bits
 */
export struct BitsInfo {
  uint16_t             address;   //!< Bits start address
  uint16_t             size;      //!< Number of bits
  std::string          name;      //!< Bits name
  BitAccess            access;    //!< Bits access
  std::vector<BitInfo> bits;      //!< Individual bits
};

/**
 * Returns information on a bits definition
 * @tparam B Bits to get information on
 * @return Information on bits
 */
template <concepts::Bits B>
BitsInfo GetBitsInfo() noexcept {
  constexpr auto Access =
      concepts::Coils<B> ? B::Options.access : BitAccess::DiscreteInput;

  if constexpr (B::Count == 1) {
    return BitsInfo{
        .address = B::Address,
        .size    = B::Count,
        .name    = std::string{static_cast<std::string_view>(B::Name)},
        .access  = Access,
        .bits    = {},
    };
  }

  std::vector<BitInfo> bits{};
  hstd::InvokeForIndexSequence<B::Count>(
      [&bits]<std::size_t Idx>(hstd::ValueMarker<Idx>) {
        constexpr auto NC = B::Options.bit_naming;
        bits.push_back({
            .address = static_cast<uint16_t>(B::StartAddress + Idx),
            .name    = std::string{static_cast<std::string_view>(
                NC.template BitName<B::Name, Idx>())},
            .access  = Access,
        });
      });

  return BitsInfo{
      .address = B::StartAddress,
      .size    = B::Count,
      .name    = std::string{static_cast<std::string_view>(B::Name)},
      .access  = Access,
      .bits    = bits,
  };
}

}   // namespace modbus::server::spec::gen