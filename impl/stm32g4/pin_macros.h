#pragma once

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
/**
 * Pin ID helper macro
 */
#define PIN(PORT, NUM)         \
::stm32g4::PinId {           \
::stm32g4::Port::PORT, NUM \
}
// NOLINTEND(cppcoreguidelines-macro-usage)