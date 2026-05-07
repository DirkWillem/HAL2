export module seq.port.system.arm_cortex_m;

namespace seq::port {

/**
 * @brief Basic system implementation for ARM Cortex-M. Uses a WFI instruction (Wait For Interrupt)
 * to wait for the next event.
 */
export struct ArmCortexMSystem {
  /** @brief Blocks until an interrupt occurs. */
  static void WaitForNextEvent() {
    asm volatile("wfi" ::: "memory");
  }
};

}   // namespace seq::port