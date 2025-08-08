module;

#include <cstdint>
#include <memory>
#include <vector>

export module hal.sil:system;

import :gpio;
import :scheduler;

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

  /**
   * Returns a reference to the scheduler
   * @return Reference to the scheduler
   */
  Scheduler& GetScheduler() & noexcept { return sched; }

 private:
  System() {}

  Scheduler sched{};

  std::vector<std::unique_ptr<Gpio>> gpios{};
};

}   // namespace sil

extern "C" {

[[maybe_unused]] std::size_t GetGpioCount() {
  return sil::System::instance().GetGpioCount();
}

[[maybe_unused]] const char* GetGpioName(std::size_t index) {
  auto& sys = sil::System::instance();

  const auto* gpio = sys.GetGpio(index);
  if (!gpio) {
    return nullptr;
  }

  return gpio->GetName().c_str();
}
}
