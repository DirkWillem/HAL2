export module hal.abstract:pin_interrupts;

import :pin;

namespace hal {

export template <typename PI>
concept PinInterrupts =
    requires(PI& impl) { requires PinId<typename PI::PinType>; };

}   // namespace hal
