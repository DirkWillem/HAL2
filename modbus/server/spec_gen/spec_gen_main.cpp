#include <iostream>

import hstd;

import modbus.server.spec;
import modbus.server.spec.gen;

using namespace modbus::server::spec::gen;
using namespace modbus::server::spec;

enum class LedState : uint16_t {
  Off               = 0,
  On                = 1,
  BlinkInPhase      = 2,
  BlinkCounterPhase = 3,
};

using LedStateDef =
    EnumDef<LedState, "LedState", EnumMember<LedState::Off, "Off">,
            EnumMember<LedState::On, "On">,
            EnumMember<LedState::BlinkInPhase, "BlinkInPhase">,
            EnumMember<LedState::BlinkCounterPhase, "BlinkCounterPhase">>;

struct StatusLedElementNaming {
  template <auto, std::size_t Idx>
    requires(Idx == 0)
  constexpr auto ElementName() const noexcept {
    return hstd::StaticString{"status_led_green"};
  }

  template <auto, std::size_t Idx>
    requires(Idx == 1)
  constexpr auto ElementName() const noexcept {
    return hstd::StaticString{"status_led_yellow"};
  }

  template <auto, std::size_t Idx>
    requires(Idx == 2)
  constexpr auto ElementName() const noexcept {
    return hstd::StaticString{"status_led_red"};
  }
};

using StatusLed =
    modbus::server::spec::HoldingRegister<0x0100, LedState, "status_led",
                                          {
                                              .enum_def = LedStateDef{},
                                          }>;

using StatusLeds = modbus::server::spec::HoldingRegister<
    0x0100, std::array<LedState, 3>, "status_leds",
    {
        .array_element_naming = StatusLedElementNaming{},
    }>;

int main() {
  const auto info = GetRegisterInfo<StatusLed>();

  std::cout << info.name << std::endl;

  for (const auto& child : info.children) {
    std::cout << child.name << ", " << child.address << std::endl;
  }

  if (info.enum_info) {
    const auto ei = *info.enum_info;
    std::cout << ei.name << std::endl;
    for (const auto& member : ei.members) {
      std::cout << "  " << member.name << std::endl;
    }
  }

  return 0;
}