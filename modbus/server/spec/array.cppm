module;

#include <string_view>
#include <tuple>
#include <utility>

export module modbus.server.spec:array;

import hstd;

namespace modbus::server::spec {

/**
 * Default naming convention for array elements, appends "_X" to the root name,
 * where X is the array index
 */
export struct DefaultArrayElementNaming {
  template <hstd::StaticString Root, std::size_t Idx>
  constexpr auto ElementName() const noexcept {
    hstd::StaticStringBuilder<Root.size() + hstd::NumDigits(Idx) + 1> result;
    result.Append(static_cast<std::string_view>(Root));
    result.Append("_");
    result.Append(Idx);
    return result;
  }
};

/**
 * Naming convention for array elements where an array of names is provided
 */
export template <hstd::StaticString... Names>
struct ArrayArrayElementNaming {
  template <hstd::StaticString, std::size_t Idx>
  constexpr auto ElementName() const noexcept {
    if (Idx >= sizeof...(Names)) {
      std::unreachable();
    }

    return std::get<Idx>(std::make_tuple(Names...));
  }
};

namespace concepts {

/**
 * Concept that describes an array element naming convention
 * @tparam Impl Implementing type
 */
export template <typename Impl>
concept ArrayElementNaming = requires(const Impl& c_impl) {
  {
    c_impl.template ElementName<hstd::StaticString{"MyArray"}, 2>()
  } -> hstd::concepts::ToStringView;
};

}   // namespace concepts

static_assert(concepts::ArrayElementNaming<DefaultArrayElementNaming>);

}   // namespace modbus::server::spec
