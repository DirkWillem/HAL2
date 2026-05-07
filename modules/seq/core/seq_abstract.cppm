module;

#include <bit>
#include <concepts>
#include <cstdint>
#include <optional>
#include <type_traits>

export module seq.abstract;

import hstd;

namespace seq {

namespace concepts {
/**
 * @brief Concept describing a numeric ID of a maximum bit width.
 * @tparam MaxSizeType Maximum Size Type.
 */
template <typename Id, typename MaxSizeType>
concept NumericId = (std::unsigned_integral<Id>
                     || (std::is_enum_v<Id> && std::is_unsigned_v<std::underlying_type_t<Id>>))
                    && sizeof(Id) <= sizeof(MaxSizeType);

/** @brief Concept describing a package ID. */
export template <typename Id>
concept PackageId = NumericId<Id, uint8_t>;

/** @brief Concept describing a module ID. */
export template <typename Id>
concept ModuleId = NumericId<Id, uint8_t>;

/** @brief Concept describing an event ID. */
export template <typename Id>
concept EventId = NumericId<Id, uint16_t>;

/** @brief Concept describing valid event data associated with a Seq event. */
export template <typename Data>
concept EventData = std::is_same_v<Data, void> || (sizeof(Data) <= 4);
}   // namespace concepts

/** @brief Event ID record. */
export struct EventIdRecord {
  uint8_t  pkg_id;   //!< Package ID.
  uint8_t  mod_id;   //!< Module ID.
  uint16_t evt_id;   //!< Event ID.
};

/** @brief Struct containing a raw event record. */
export struct EventRecord {
  uint32_t timestamp;   //!< Event timestamp.
  uint32_t id;          //!< Event ID.
  uint32_t data;        //!< Event data.
};

namespace concepts {

/**
 * @brief Concept describing a queue used for the event queue in Seq.
 * @tparam T Queue element type.
 */
export template <typename Q, typename T>
concept Queue = requires(Q& queue) {
  { queue.Push(std::declval<const T&>()) } -> std::convertible_to<bool>;
  { queue.Pop() } -> std::convertible_to<std::optional<T>>;
};

/** @brief Concept describing a system implementation for Seq. */
export template <typename Sys>
concept System = requires() { Sys::WaitForNextEvent(); };

/** @brief Concept describing a valid OS settings struct for Seq. */
export template <typename Settings>
concept OsSettings = requires(Settings& s) {
  requires hstd::Duration<typename std::decay_t<Settings>::Duration>;
  requires Queue<typename std::decay_t<Settings>::EventQueue, EventRecord>;
  requires System<typename std::decay_t<Settings>::System>;
};

/** @brief Concept describing a Seq event sink. */
export template <typename EvSink>
concept EventSink =
    requires() { EvSink::Push(std::declval<uint32_t>(), std::declval<uint32_t>()); };

export template <typename Mod>
concept Module = requires(Mod& mod) { mod(std::declval<uint32_t>(), std::declval<uint32_t>()); };

}   // namespace concepts

export template <concepts::PackageId auto PkgId, concepts::ModuleId auto ModId,
                 concepts::EventId auto EvtId, concepts::EventData EvtData = void>
struct Event {
  /** @brief Numeric ID of the event. */
  static constexpr uint32_t Id = [] {
    EventIdRecord id_rec{static_cast<uint8_t>(PkgId), static_cast<uint8_t>(ModId),
                         static_cast<uint16_t>(EvtId)};
    return std::bit_cast<uint32_t>(id_rec);
  }();

  /** @brief Event data type. */
  using Data = EvtData;

  /**
   * @brief Decodes raw event data to the data type of this event.
   * @param raw_data Raw data to convert.
   * @return Event data.
   */
  [[nodiscard]] constexpr static Data GetData(uint32_t raw_data) noexcept
    requires(!std::is_same_v<Data, void>)
  {
    return std::bit_cast<Data>(raw_data);
  }

  /**
   * @brief Emits an event into the given event sink.
   * @tparam Sink Sink to emit the event to.
   * @param data Event data to emit.
   */
  template <concepts::EventSink Sink>
    requires(!std::is_same_v<EvtData, void>)
  static void
  Emit(std::conditional_t<std::is_same_v<EvtData, void>, unsigned, EvtData> data) noexcept {
    Sink::Push(Id, std::bit_cast<uint32_t>(data));
  }

  /**
   * @brief Emits an event into the given event sink.
   * @tparam Sink Sink to emit the event to.
   */
  template <concepts::EventSink Sink>
    requires(std::is_same_v<EvtData, void>)
  static void Emit() noexcept {
    Sink::Push(Id, 0);
  }
};

}   // namespace seq