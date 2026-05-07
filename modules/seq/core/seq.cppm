module;

#include <chrono>

export module seq;

export import seq.abstract;

namespace seq {

/**
 * @brief Default base OS settings
 * @tparam Sys \c seq::concepts::System implementation to use.
 * @tparam Queue \c seq::concepts::Queue implementation to use.
 */
export template <concepts::System Sys, concepts::Queue<EventRecord> Queue>
struct DefaultOsSettings {
  using Duration   = std::chrono::duration<uint32_t, std::micro>;   //!< Duration type.
  using EventQueue = Queue;                                         //!< Event queue implementation.
  using System     = Sys;
};

/**
 * @brief Seq OS.
 * @tparam S OS settings.
 */
export template <concepts::OsSettings S>
class Os {
  using Settings = S;

 public:
  /** @brief Event sink for this OS type. */
  class EventSink {
    friend class Os;

   public:
    /**
     * @brief Pushes an event to the event queue.
     * @param event_id ID of the event to push.
     * @param event_data Data corresponding to the event to push.
     */
    static void Push(uint32_t event_id, uint32_t event_data) {
      (*GetOsPtr())
          ->queue.Push(EventRecord{
              .timestamp = 0,
              .id        = event_id,
              .data      = event_data,
          });
    }

   protected:
    /**
     * @brief Registers an OS instance with the event sink.
     * @param os Reference to the OS to register with the event sink.
     */
    static void RegisterOs(Os& os) { *GetOsPtr() = &os; }

    /**
     * @brief De-registers a previously registered OS with the event sink.
     */
    static void DeregisterOs() { *GetOsPtr() = nullptr; }

   private:
    /**
     * @brief Returns a pointer to the pointer to the OS instance.
     * @return Pointer to the pointer to the OS instance.
     */
    static Os** GetOsPtr() {
      static Os* ptr{nullptr};
      return &ptr;
    }
  };

  /** @brief Constructor. */
  Os() { EventSink::RegisterOs(*this); }
  /** @brief Destructor. */
  ~Os() { EventSink::DeregisterOs(); }

  /**
   * @brief Enters the main OS loop.
   * @tparam Modules Modules to run in the OS.
   * @param modules Modules to run in the OS.
   */
  template <concepts::Module... Modules>
  [[noreturn]] void Run(Modules&... modules) {
    while (true) {
      queue.ReadAll(
          [&modules...](const EventRecord& event) { (..., modules(event.id, event.data)); });
      Settings::System::WaitForNextEvent();
    }
  }

 private:
  Settings::EventQueue queue{};
};

}   // namespace seq