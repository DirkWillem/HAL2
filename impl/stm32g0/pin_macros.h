#pragma once

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
/**
 * Pin ID helper macro
 */
#define PIN(PORT, NUM)         \
  ::stm32g0::PinId {           \
    ::stm32g0::Port::PORT, NUM \
  }
// NOLINTEND(cppcoreguidelines-macro-usage)