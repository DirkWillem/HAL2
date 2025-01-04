#pragma once

#include <atomic>
#include <concepts>

namespace halstd {

template <typename Impl>
concept Atomic = requires(Impl t) {
  typename Impl::value_type;

  { t.load() } -> std::convertible_to<typename Impl::value_type>;
  {
    t.load(std::declval<std::memory_order>())
  } -> std::convertible_to<typename Impl::value_type>;

  { t.store(std::declval<typename Impl::value_type>()) };
  {
    t.store(std::declval<typename Impl::value_type>(),
            std::declval<std::memory_order>())
  };

  {
    t.compare_exchange_strong(std::declval<typename Impl::value_type&>(),
                              std::declval<typename Impl::value_type>())
  } -> std::convertible_to<bool>;
  {
    t.compare_exchange_strong(std::declval<typename Impl::value_type&>(),
                              std::declval<typename Impl::value_type>(),
                              std::declval<std::memory_order>())
  } -> std::convertible_to<bool>;
};

static_assert(Atomic<std::atomic<int>>);

}   // namespace halstd