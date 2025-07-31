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
        .enum_def             = LedStateDef{},
    }>;

using AxisEnables =
    Coils<0x0100, 2, "axis_control_en",
          {
              .bit_naming = ArrayBitNaming<"panel1_x_control_en",
                                           "panel1_y_control_en">{},
          }>;

using Server = ServerSpec<hstd::Types<>, hstd::Types<AxisEnables>,
                          hstd::Types<>, hstd::Types<StatusLeds>>;

int main() {
  const auto spec = GetServerSpecJson<Server>();

  std::cout << spec.dump(2) << std::endl;

  return 0;
}