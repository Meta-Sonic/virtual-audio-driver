#pragma once
#include <CoreAudio/AudioServerPlugIn.h>
#include "mts/util.h"
#include "mts/dsp.h"
#include <dispatch/dispatch.h>
#include <mach/mach_time.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/syslog.h>
#include <stdlib.h>
#include <array>

namespace mts {
enum class direction { input, output };

enum class object_type { plugin, box, device, stream, control };

struct object_description {
  AudioObjectID id;
  object_type type;
  direction direction;
};

class object {
public:
  using Address = AudioObjectPropertyAddress;

  inline object(AudioObjectID objID)
      : _id(objID) {}

  inline AudioObjectID get_id() const { return _id; }

private:
  AudioObjectID _id;
};

class mutex {
public:
  inline void lock() { pthread_mutex_lock(&_mutex); }

  inline void unlock() { pthread_mutex_unlock(&_mutex); }

private:
  pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
};

class scoped_lock {
public:
  inline explicit scoped_lock(mutex& m)
      : _mutex(m) {
    _mutex.lock();
  }

  inline ~scoped_lock() { _mutex.unlock(); }

  scoped_lock(scoped_lock const&) = delete;
  scoped_lock& operator=(scoped_lock const&) = delete;

private:
  mutex& _mutex;
};

template <typename T>
inline UInt32 assign(void* dst, const T& src) {
  static_assert(!std::is_same_v<T, CFStringRef>, "");

  (*(T*)dst) = src;
  return sizeof(T);
}

template <>
inline UInt32 assign<CFStringRef>(void* dst, const CFStringRef& src) {
  CFStringRef& dst_str = (*(CFStringRef*)dst);
  dst_str = src;

  if (dst_str) {
    CFRetain(dst_str);
  }

  return sizeof(CFStringRef);
}

} // namespace mts.

#define MTS_API __attribute__((visibility("default")))
#define MTS_HIDDEN __attribute__((visibility("hidden")))

#if DEBUG
  #define MTS_TRACE() syslog(LOG_NOTICE, "MTS_DRIVER: %s in file %s(%d)", __FUNCTION__, __FILE__, __LINE__)
  #define MTS_DBG(msg) syslog(LOG_NOTICE, "MTS_DRIVER: %s in file %s(%d) : %s", __FUNCTION__, __FILE__, __LINE__, msg)
#else
  #define MTS_TRACE()
  #define MTS_DBG(msg)
#endif // DEBUG.

#define RETURN_ERROR_IF(cond, retValue, msg)                                                                           \
  do {                                                                                                                 \
    if ((cond)) {                                                                                                      \
      MTS_DBG(msg);                                                                                                    \
      return retValue;                                                                                                 \
    }                                                                                                                  \
  } while (false)

#define RETURN_SIZE_ERROR_IF(cond)                                                                                     \
  do {                                                                                                                 \
    if ((cond)) {                                                                                                      \
      return kAudioHardwareBadPropertySizeError;                                                                       \
    }                                                                                                                  \
  } while (false)

#define RETURN_FORMAT_ERROR_IF(cond)                                                                                   \
  do {                                                                                                                 \
    if ((cond)) {                                                                                                      \
      return kAudioDeviceUnsupportedFormatError;                                                                       \
    }                                                                                                                  \
  } while (false)
