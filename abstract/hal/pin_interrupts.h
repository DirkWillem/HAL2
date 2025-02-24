#pragma once

#include "pin.h"

#include <halstd/callback.h>

namespace hal {

template <typename PI>
concept PinInterrupts =
    requires(PI& impl) { requires PinId<typename PI::PinType>; };

}   // namespace hal