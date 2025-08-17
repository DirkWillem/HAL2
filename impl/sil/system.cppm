module;

#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <span>
#include <vector>

export module hal.sil:system;

import :gpio;
import :scheduler;
import :uart;

namespace sil {

export class System {
  friend std::unique_ptr<System> std::make_unique<System>();

  static std::unique_ptr<System>& instance_ptr() noexcept {
    static std::unique_ptr<System> inst_ptr{nullptr};
    return inst_ptr;
  }

 public:
  static System& instance() noexcept {
    auto& inst_ptr = instance_ptr();
    if (inst_ptr == nullptr) {
      inst_ptr = std::make_unique<System>();
    }
    return *inst_ptr;
  }

  static void Reset() { instance_ptr().reset(); }

  /**
   * Sets an error callback for the system
   * @param cb Error callback to register
   */
  void SetErrorCallback(std::function<void(const char*)> cb) {
    error_callback = cb;
  }

  /**
   * Clears the set error callback for the system
   */
  void ClearErrorCallback() { error_callback = {}; }

  void HandleError(const std::string& str) {
    if (error_callback) {
      (*error_callback)(str.c_str());
    }
  }

  void HandleException(const std::exception& e) {
    if (error_callback) {
      (*error_callback)(e.what());
    }
  }

  Gpio& DefineGpio(std::string_view name, GpioDirection direction,
                   bool initial_state = false) {
    return *gpios.emplace_back(
        std::make_unique<Gpio>(std::string{name}, direction, initial_state));
  }

  [[nodiscard]] std::size_t GetGpioCount() const noexcept {
    return gpios.size();
  }

  Gpio* GetGpio(std::size_t index) {
    if (index >= gpios.size()) {
      return nullptr;
    }
    return gpios[index].get();
  }

  template <std::derived_from<Uart> U>
  U& DefineUart(std::string_view name, unsigned baud = 115'200,
                hal::UartParity   parity    = hal::UartParity::None,
                hal::UartStopBits stop_bits = hal::UartStopBits::One) {
    return static_cast<U&>(*uarts.emplace_back(
        std::make_unique<U>(sched, name, baud, parity, stop_bits)));
  }

  std::size_t GetUartCount() const noexcept { return uarts.size(); }

  Uart* GetUart(std::size_t index) {
    if (index >= uarts.size()) {
      return nullptr;
    }
    return uarts[index].get();
  }

  /**
   * Returns a reference to the scheduler
   * @return Reference to the scheduler
   */
  Scheduler& GetScheduler() & noexcept { return sched; }

 private:
  System() {}

  Scheduler sched{};

  std::vector<std::unique_ptr<Gpio>> gpios{};
  std::vector<std::unique_ptr<Uart>> uarts{};

  std::optional<std::function<void(const char*)>> error_callback{};
};

}   // namespace sil

using ErrorCallback        = void (*)(const char*);
using UartTransmitCallback = void (*)(const uint8_t*, std::size_t);

/**
 * Helper function for performing an action with a UART given its index
 * @tparam T Return type
 * @param index UART index
 * @param err_value Value to return in case the UART could not be found, or an
 * error occurs
 * @param inner Inner function
 * @return Result of inner function, or error value in case of a failure
 */
template <typename T>
T WithUart(std::size_t index, T err_value,
           std::invocable<sil::Uart&> auto inner) {
  auto& sys = sil::System::instance();

  try {
    auto* uart = sys.GetUart(index);

    if (!uart) {
      sys.HandleError(std::format("{} is not a valid UART index", index));
      return err_value;
    }

    return inner(*uart);
  } catch (std::exception& e) {
    sys.HandleException(e);
    return err_value;
  }
}

extern "C" {

[[maybe_unused]] void SetErrorCallback(ErrorCallback cb) {
  sil::System::instance().SetErrorCallback(cb);
}

[[maybe_unused]] void ClearErrorCallback() {
  sil::System::instance().ClearErrorCallback();
}

[[maybe_unused]] void Sched_Start() {
  auto& sys = sil::System::instance();

  try {
    sys.GetScheduler().Start();
  } catch (std::exception& e) {
    sys.HandleException(e);
  }
}

[[maybe_unused]] void Sched_RunUntil(uint64_t time_us) {
  auto& sys = sil::System::instance();

  try {
    sys.GetScheduler().RunUntil(sil::TimePointUs{time_us});
  } catch (std::exception& e) {
    sys.HandleException(e);
  }
}

[[nodiscard]] bool Sched_RunUntilNextTimePoint(uint64_t upper_bound) {
  auto& sys = sil::System::instance();

  try {
    return sys.GetScheduler().RunUntilNextTimePoint(
        sil::TimePointUs{upper_bound});
  } catch (std::exception& e) {
    sys.HandleException(e);
    return false;
  }
}

[[maybe_unused]] void Sched_Shutdown(std::size_t max_iters) {
  auto& sys = sil::System::instance();

  try {
    sys.GetScheduler().Shutdown(max_iters);
  } catch (std::exception& e) {
    sys.HandleException(e);
  }
}

[[maybe_unused]] uint64_t Sched_Now() {
  auto& sys = sil::System::instance();
  return sys.GetScheduler().Now().count();
}

[[maybe_unused]] std::size_t Gpio_GetGpioCount() {
  auto& sys = sil::System::instance();

  try {
    return sys.GetGpioCount();
  } catch (std::exception& e) {
    sys.HandleException(e);
    return 0;
  }
}

[[maybe_unused]] const char* Gpio_GetGpioName(std::size_t index) {
  auto& sys = sil::System::instance();

  try {
    const auto* gpio = sys.GetGpio(index);
    if (!gpio) {
      sys.HandleError(std::format("{} is not a valid GPIO index", index));
      return nullptr;
    }

    return gpio->GetName().c_str();
  } catch (std::exception& e) {
    sys.HandleException(e);
    return nullptr;
  }
}

[[maybe_unused]] std::size_t Uart_GetUartCount() {
  auto& sys = sil::System::instance();

  try {
    return sys.GetUartCount();
  } catch (std::exception& e) {
    sys.HandleException(e);
    return 0;
  }
}

[[maybe_unused]] const char* Uart_GetUartName(std::size_t index) {
  return WithUart<const char*>(
      index, nullptr, [](auto& uart) { return uart.GetName().c_str(); });
}

[[maybe_unused]] bool Uart_SimulateReceive(std::size_t    index,
                                           uint64_t       timestamp_us,
                                           const uint8_t* data,
                                           std::size_t    data_len) {
  return WithUart(index, false, [timestamp_us, data, data_len](auto& uart) {
    uart.SimulateRx(
        sil::TimePointUs{timestamp_us},
        hstd::ReinterpretSpan<std::byte>(std::span{data, data_len}));
    return true;
  });
}

[[maybe_unused]] bool Uart_SetTransmitCallback(std::size_t          index,
                                               UartTransmitCallback cb) {
  return WithUart(index, false, [cb](auto& uart) {
    uart.SetTxCallback([cb](std::span<const std::byte> data) {
      const auto data_u8 = hstd::ReinterpretSpan<uint8_t>(data);
      (*cb)(data_u8.data(), data_u8.size());
    });
    return true;
  });
}
}
