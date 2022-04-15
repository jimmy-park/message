#ifndef TYPE_TRAITS_H_
#define TYPE_TRAITS_H_

#include <type_traits>

namespace type_traits {

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename T>
inline constexpr bool always_false_v = false;

template <typename T>
inline constexpr bool is_vector_v = false;

template <typename T>
inline constexpr bool is_vector_v<std::vector<T>> = true;

} // namespace type_traits

#endif // TYPE_TRAITS_H_