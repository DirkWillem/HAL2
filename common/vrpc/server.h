#pragma once

#include <optional>
#include <memory>

#include <halstd/callback.h>

namespace vrpc {

template <typename Response>
class AsyncResult {
  struct State {
    std::reference_wrapper<Response>           response;
    std::reference_wrapper<halstd::Callback<>> callback;
  };

 public:
  /**
   * Default constructor, initializes to a non-completable result
   */
  AsyncResult() noexcept
      : state{} {}

  /**
   * Constructor
   * @param response Reference to the response message
   * @param callback Reference to the complete callback
   */
  AsyncResult(Response& response, halstd::Callback<>& callback) noexcept
      : state{
            {.response = std::ref(response), .callback = std::ref(callback)}} {}

  /**
   * Move constructor
   * @param rhs Right-hand side
   */
  AsyncResult(AsyncResult&& rhs) noexcept {
    state = std::move(rhs.state);
    rhs.state.reset();
  }

  /**
   * Move assignment operator
   * @param rhs Right-hand side
   * @return Reference to this
   */
  AsyncResult& operator=(AsyncResult&& rhs) noexcept {
    state = std::move(rhs.state);
    rhs.state.reset();
    return *this;
  }

  // Delete copy constructor and assignment operator
  AsyncResult(const AsyncResult&)            = delete;
  AsyncResult& operator=(const AsyncResult&) = delete;

  /**
   * Edits the response message without completing the result
   * @param action Action to apply to the response
   * @return Whether editing was successful
   */
  bool EditResponse(std::invocable<Response&> auto&& action) noexcept {
    if (!state.has_value()) {
      return false;
    }

    action(state->response.get());
    return true;
  }

  /**
   * Completes the asynchronous operation
   * @return Whether completion was successful
   */
  bool Complete() noexcept {
    if (!state.has_value()) {
      return false;
    }

    state->callback.get()();
    state.reset();
    return true;
  }

  /**
   * Applies an edit to the response message and completes the asynchronous
   * operation
   * @param action Action to apply to the response message
   * @return Whether completion was successful
   */
  bool Complete(std::invocable<Response&> auto action) noexcept {
    if (!state.has_value()) {
      return false;
    }

    action(state->response.get());
    state->callback.get()();
    state.reset();

    return true;
  }

  /**
   * returns whether the asynchronous operation was completed
   * @return Whether the asynchronous operation was completed
   */
  [[nodiscard]] bool completed() const noexcept { return !state.has_value(); }

 private:
  std::optional<State> state;
};

}   // namespace vrpc