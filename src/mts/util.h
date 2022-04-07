#pragma once
#include <type_traits>
#include <math.h>

namespace mts {
template <typename T>
T clamp(T d, T min, T max) {
  const T t = d < min ? min : d;
  return t > max ? max : t;
}

template <typename T0, typename T1, typename... Ts>
inline constexpr typename std::common_type<T0, T1, Ts...>::type min(T0 v1, T1 v2, Ts... vs) {
  if constexpr (sizeof...(Ts) == 0) {
    return v2 < v1 ? v2 : v1;
  }
  else {

    return v2 < v1 ? min(v2, vs...) : min(v1, vs...);
  }
}

template <typename T0, typename T1, typename... Ts>
inline constexpr typename std::common_type<T0, T1, Ts...>::type max(T0 v1, T1 v2, Ts... vs) {
  if constexpr (sizeof...(Ts) == 0) {
    return v2 > v1 ? v2 : v1;
  }
  else {
    return v2 > v1 ? max(v2, vs...) : max(v1, vs...);
  }
}

template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
inline constexpr T is_power_of_two(T v) {
  return v && !(v & (v - 1));
}

/// Check if the first value is the same as one of the other ones.
///
/// These two conditions are equivalent:
/// @code
///   if(is_one_of(a, 0, 1, 2, 3)) {
///     ...
///   }
///
///   if(a == 0 || a == 1 || a == 2 || a == 3) {
///     ...
///   }
/// @endcode
template <typename T, typename T1, typename... Ts>
inline constexpr bool is_one_of(T t, T1 t1, Ts... ts) {
  if constexpr (sizeof...(Ts) == 0) {
    return t == t1;
  }
  else {
    return (t == t1) || is_one_of(t, ts...);
  }
}

template <typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
inline T amplitude_to_decibel(T amp) {
  return T(20.0) * log10(amp < T(1e-23) ? T(1e-23) : amp);
}

template <typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
inline T decibel_to_amplitude(T db) {
  return pow((T)10.0, db / (T)20.0);
}

template <typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
inline T amplitude_to_normalized_value(T amp, T minDB, T maxDB) {
  const T db = clamp(amplitude_to_decibel(amp), minDB, maxDB);
  return (db - minDB) / (maxDB - minDB);
}

template <typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
inline T normalized_value_to_amplitude(T value, T minDB, T maxDB) {
  const T db = minDB + clamp(value, (T)0.0, (T)1.0) * (maxDB - minDB);
  return decibel_to_amplitude(db);
}
} // namespace mts.
