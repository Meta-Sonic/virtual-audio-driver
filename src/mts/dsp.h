#pragma once
#include "mts/util.h"
#include <string.h>
#include <Accelerate/Accelerate.h>

namespace mts::dsp {
/// Clear a buffer of floating points.
/// TODO: Benchmark agains vDSP_vclr(...)
template <typename T, std::enable_if_t<std::is_floating_point_v<T> && is_one_of(sizeof(T), 4, 8), bool> = true>
inline void clear(T* buffer, size_t size) {
  if constexpr (sizeof(T) == 4) {
    const T val = 0;
    memset_pattern4((void*)buffer, (const void*)&val, size * sizeof(T));
  }
  else if constexpr (sizeof(T) == 8) {
    const T val = 0;
    memset_pattern8((void*)buffer, (const void*)&val, size * sizeof(T));
  }
}

/// Copy a buffer of floating points.
/// TODO: Benchmark agains cblas_scopy(...)
template <typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
inline void copy(const T* src, T* dst, size_t size) {
  memcpy((void*)dst, (const void*)src, size * sizeof(T));
}

/// Multiply vector with value.
template <typename T, std::enable_if_t<std::is_floating_point_v<T> && is_one_of(sizeof(T), 4, 8), bool> = true>
inline void mul(T* buffer, T value, size_t size) {
  if constexpr (sizeof(T) == 4) {
    vDSP_vsmul((const T*)buffer, 1, (const T*)&value, buffer, 1, size);
  }
  else if constexpr (sizeof(T) == 8) {
    vDSP_vsmulD((const T*)buffer, 1, (const T*)&value, buffer, 1, size);
  }
}
} // namespace mts::dsp.
