module;

#include <cstdint>
#include <memory>
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

  Gpio& DefineGpio(std::string_view name, GpioDirection direction,
                   bool initial_state = false) {
    return *gpios.emplace_back(
        std::make_unique<Gpio>(std::string{name}, direction, initial_state));
  }

  std::size_t GetGpioCount() const noexcept { return gpios.size(); }

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
};

}   // namespace sil

extern "C" {

[[maybe_unused]] void Sched_Start() {
  sil::System::instance().GetScheduler().Start();
}

[[maybe_unused]] void Sched_RunUntil(uint64_t time_us) {
  sil::System::instance().GetScheduler().RunUntil(sil::TimePointUs{time_us});
}

[[maybe_unused]] void Sched_Shutdown(std::size_t max_iters) {
  sil::System::instance().GetScheduler().Shutdown(max_iters);
}

[[maybe_unused]] std::size_t Gpio_GetGpioCount() {
  return sil::System::instance().GetGpioCount();
}

[[maybe_unused]] const char* Gpio_GetGpioName(std::size_t index) {
  auto& sys = sil::System::instance();

  const auto* gpio = sys.GetGpio(index);
  if (!gpio) {
    return nullptr;
  }

  return gpio->GetName().c_str();
}

[[maybe_unused]] std::size_t Uart_GetUartCount() {
  return sil::System::instance().GetUartCount();
}

[[maybe_unused]] const char* Uart_GetUartName(std::size_t index) {
  auto&       sys  = sil::System::instance();
  const auto* uart = sys.GetUart(index);
  if (!uart) {
    return nullptr;
  }
  return uart->GetName().c_str();
}

[[maybe_unused]] bool Uart_SimulateReceive(std::size_t index,
                                           uint64_t    timestamp_us,
                                           const char* data,
                                           std::size_t data_len) {
  auto& sys  = sil::System::instance();
  auto* uart = sys.GetUart(index);

  if (!uart) {
    return false;
  }

  uart->SimulateRx(sil::TimePointUs{timestamp_us},
                   hstd::ReinterpretSpan<std::byte>(std::span{data, data_len}));
  return true;
}
}
