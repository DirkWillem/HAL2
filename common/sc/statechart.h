#pragma once

#include <atomic>
#include <variant>

#include <halstd/atomic_helpers.h>
#include <halstd/mp/type_helpers.h>
#include <halstd/mp/types.h>

namespace sc {
template <typename... S>
struct States : halstd::Types<S...> {};

template <typename... E>
struct Events : halstd::Types<E...> {};

template <typename S, typename E, typename D>
struct Transition {
  using Src   = std::remove_cvref_t<S>;
  using Dst   = D;
  using Event = std::remove_cvref_t<E>;
};

template <typename T>
struct TransitionTraits : std::false_type {};

template <typename T>
  requires requires { &T::operator(); }
struct TransitionTraits<T> : TransitionTraits<decltype(&T::operator())> {};

template <typename T, typename S, typename E, typename D>
struct TransitionTraits<auto (T::*)(S, E)->D> : std::true_type {
  using Tr = Transition<S, E, D>;
};

template <typename T, typename S, typename E, typename D>
struct TransitionTraits<auto (T::*)(S, E) const->D> : std::true_type {
  using Tr = Transition<S, E, D>;
};

namespace detail {

template <typename Src, typename Event, typename... Trs>
struct FindTransitionHelper;

template <typename Src, typename Event, typename Cur, typename... Rest>
struct FindTransitionHelper<Src, Event, Cur, Rest...> {
  static constexpr auto IsMatch = std::is_same_v<typename Cur::Src, Src>
                                  && std::is_same_v<typename Cur::Event, Event>;

  using Result = std::conditional_t<
      IsMatch, Cur, typename FindTransitionHelper<Src, Event, Rest...>::Result>;
};

template <typename Src, typename Event>
struct FindTransitionHelper<Src, Event> {
  using Result = void;
};

template <typename... Ts>
struct TransitionList {
  template <typename Src, typename Event>
  static consteval bool ContainsTransition() {
    return (...
            || (std::is_same_v<typename Ts::Src, Src>
                && std::is_same_v<typename Ts::Event, Event>));
  }

  template <typename Src, typename Event>
  using FindTransition = typename FindTransitionHelper<Src, Event>::Result;
};

template <typename T>
struct IsInState {
  [[nodiscard]] constexpr bool operator()(const T&) const noexcept {
    return true;
  }

  template <typename U>
    requires(!std::is_same_v<std::decay_t<T>, std::decay_t<U>>)
  [[nodiscard]] constexpr bool operator()(U) const noexcept {
    return false;
  }
};

template <typename T, std::invocable<const T&> F>
struct TryGetStateValue {
  F fn;

  using Result = decltype(fn(std::declval<const T&>()));

  [[nodiscard]] constexpr std::optional<Result>
  operator()(const T& s) const noexcept {
    return fn(s);
  }

  template <typename U>
    requires(!std::is_same_v<std::decay_t<T>, std::decay_t<U>>)
  constexpr std::optional<Result> operator()(const U&) const noexcept {
    return {};
  }
};

}   // namespace detail

template <typename... Ts>
struct Transitions : Ts... {
  using TransitionList =
      detail::TransitionList<typename TransitionTraits<Ts>::Tr...>;
};

template <typename... Ts>
Transitions(Ts...) -> Transitions<Ts...>;

template <typename SL, typename EL>
struct StateChart;

template <typename SC>
class StateChartRunner;

template <typename... Ss, typename... Es>
struct StateChart<States<Ss...>, Events<Es...>> {
  template <typename Tr>
  class Chart;

  template <typename... Trs>
  class Chart<Transitions<Trs...>> : Trs... {
    static constexpr std::size_t NStates = sizeof...(Ss);
    static constexpr std::size_t NEvents = sizeof...(Es);
    using TrList = typename Transitions<Trs...>::TransitionList;

    template <typename T>
    friend class StateChartRunner;

    using Trs::operator()...;

   public:
    template <typename Si>
      requires(... || std::is_same_v<std::decay_t<Si>, Ss>)
    explicit Chart(Si initial_state, Transitions<Trs...>) noexcept
        : state{initial_state} {}

   protected:
    using Event = std::variant<Es...>;

    template <typename S, typename E>
    void Apply(const std::variant<Es...>& event) noexcept {
      const E& e = std::get<E>(event);
      const S& s = std::get<S>(state);

      state = (*this)(s, e);
    }

    template <typename S>
      requires(... || std::is_same_v<S, Ss>)
    [[nodiscard]] static consteval std::size_t StateIndex() noexcept {
      return *halstd::Types<Ss...>::template IndexOf<S>();
    }

    template <typename E>
      requires(... || std::is_same_v<E, Es>)
    [[nodiscard]] static consteval std::size_t EventIndex() noexcept {
      return *halstd::Types<Es...>::template IndexOf<E>();
    }

    template <typename E>
    [[nodiscard]] static constexpr bool IsValidEvent() noexcept {
      return (... || std::is_same_v<E, Es>);
    }

    template <typename S>
    [[nodiscard]] static constexpr bool IsValidState() noexcept {
      return (... || std::is_same_v<S, Ss>);
    }

    template <typename S, typename E>
    static consteval void BuildJumpTableForStateAndEvent(
        std::array<
            std::array<void (Chart::*)(const std::variant<Es...>&), NEvents>,
            NStates>& table) {
      using StateList = halstd::Types<Ss...>;
      using EventList = halstd::Types<Es...>;
      if constexpr (TrList::template ContainsTransition<S, E>()) {
        // const auto SI = *;
        table[*StateList::template IndexOf<S>()]
             [*EventList::template IndexOf<E>()] =
                 static_cast<void (Chart::*)(const std::variant<Es...>&)>(
                     &Chart::template Apply<S, E>);
      }
    }

    template <typename S>
    static consteval void BuildJumpTableForState(
        std::array<
            std::array<void (Chart::*)(const std::variant<Es...>&), NEvents>,
            NStates>& table) {
      (..., BuildJumpTableForStateAndEvent<S, Es>(table));
    }

    [[nodiscard]] static consteval auto BuildTransitionTable() noexcept {
      std::array<
          std::array<void (Chart::*)(const std::variant<Es...>&), NEvents>,
          NStates>
          table{};

      (..., BuildJumpTableForState<Ss>(table));
      return table;
    }

    std::variant<Ss...> state;
  };

  template <typename Si, typename Tr>
  Chart(Si, Tr) -> Chart<Tr>;
};

template <typename SC>
class StateChartRunner {
 public:
  static constexpr auto JumpTable = SC::BuildTransitionTable();

  explicit StateChartRunner(SC chart)
      : chart{chart} {}

  /**
   * Immediately applies an event to the state chart. This method should not
   * be used in an ISR context. In that case, use EnqueueEvent instead
   * @tparam E Event type
   * @param event Event to apply
   * @return Whether applying the event was successful
   */
  template <typename E>
    requires(SC::template IsValidEvent<E>())
  [[nodiscard]] bool ApplyEvent(E event) noexcept {
    const auto inner = [&, this] {
      // Optionally process any pending event
      if (has_enqueued_event.test_and_set()) {
        if (enqueued_event.has_value()) {
          const auto& ee = *enqueued_event;

          const auto ee_idx = ee.index();
          if (const auto tr_method = JumpTable[chart.state.index()][ee.index()];
              tr_method != nullptr) {
            (chart.*tr_method)(ee);
          }
        }

        enqueued_event.reset();
        has_enqueued_event.clear();
      }

      // Process the incoming event
      constexpr auto Ei = SC::template EventIndex<std::decay_t<E>>();
      if (const auto tr_method = JumpTable[chart.state.index()][Ei];
          tr_method != nullptr) {
        (chart.*tr_method)(event);
        return true;
      }

      return false;
    };

    return halstd::ExclusiveWithAtomicFlag(processing_event, inner)
        .value_or(false);
  }

  /**
   * Enqueues an event to be processed later. This method should typically
   * be used from an ISR context
   * @tparam E Event type
   */
  template <typename E>
    requires(SC::template IsValidEvent<E>())
  void EnqueueEvent(E event) noexcept {
    constexpr auto Ei = SC::template EventIndex<std::decay_t<E>>();

    if (!has_enqueued_event.test_and_set()) {
      enqueued_event = event;
    }
  }

  /**
   * Processes any event that was previously enqueued by EnqueueEvent
   */
  void ProcessEnqueuedEvent() noexcept {
    if (has_enqueued_event.test_and_set()) {
      const auto inner = [&, this] {
        if (enqueued_event.has_value()) {
          const auto& ee = *enqueued_event;

          const auto ee_idx = ee.index();
          if (const auto tr_method = JumpTable[chart.state.index()][ee.index()];
              tr_method != nullptr) {
            (chart.*tr_method)(ee);
          }
        }

        enqueued_event.reset();
        has_enqueued_event.clear();
      };

      halstd::ExclusiveWithAtomicFlag(processing_event, inner);
    } else {
      has_enqueued_event.clear();
    }
  }

  /**
   * Applies a visitor to the current state of the statechart and returns
   * the result
   * @tparam V Visitor type
   * @param visitor Visitor to apply
   * @return Visitor result
   */
  template <typename V>
  auto Visit(V visitor) const noexcept {
    return std::visit(visitor, chart.state);
  }

 private:
  SC                                chart;
  std::optional<typename SC::Event> enqueued_event{};

  std::atomic_flag has_enqueued_event{};
  std::atomic_flag processing_event{};
};

namespace detail {

template <typename T>
struct IsStateChartRunnerHelper : std::false_type {};

template <typename T>
struct IsStateChartRunnerHelper<StateChartRunner<T>> : std::true_type {};

}   // namespace detail

template <typename T>
concept IsStateChartRunner = (detail::IsStateChartRunnerHelper<T>::value);

/**
 * Visitor that returns whether a state chart is in the given state
 * @tparam T State to check
 * @return Visitor
 */
template <typename T>
constexpr auto IsInState() noexcept {
  return detail::IsInState<T>{};
}

/**
 * Visitor that extracts a value from a given state using the given
 * function and wraps it in a std::optional. If the visitor is in a different
 * state, returns std::nullopt
 * @tparam T State to extract the value from
 * @param fn Function to apply to the state to obtain the extracted value
 * @return Visitor
 */
template <typename T>
constexpr auto TryGetStateValue(std::invocable<const T&> auto fn) noexcept {
  return detail::TryGetStateValue<T, std::decay_t<decltype(fn)>>{fn};
}

template <typename Src, typename Event, typename Dst>
constexpr auto MakeTransition() noexcept {
  return [](Src, Event) { return Dst{}; };
}

}   // namespace sc
