#include "config.h"
#include "mts/common.h"
#include "mts/object/mute_control.h"
#include "mts/object/volume_control.h"
#include "mts/object/box.h"
#include "mts/object/device.h"
#include "mts/object/stream.h"
#include "mts/object/plugin.h"

//
// Config validation.
//
namespace mts::config {
static_assert(bits_per_channel == 32, "only 32 bits is currently supported");
static_assert(is_power_of_two(ring_buffer_size), "ringBufferSize must be a power of two");
static_assert(is_power_of_two(ring_buffer_frame_size), "ringBufferFrameSize must be a power of two");
static_assert(is_default_sample_rate_supported(), "defaultSampleRate must be a supported sample rate");
static_assert(is_all_sample_rate_integers(), "supported sample rates must be integers");
} // namespace mts::config.

/// The plug-in is responsible for defining the AudioObjectIDs to be used as handles for the
/// AudioObjects the plug-in provides. However, the AudioObjectID for the one and only AudioPlugIn
/// object must be kAudioObjectPlugInObject.
enum class ObjectID : AudioObjectID {
  Plugin = kAudioObjectPlugInObject,
  Box,
  Device,

  // Device input scope.
  StreamInput,
  VolumeInputMaster,
  MuteInputMaster,

  // Device output scope.
  StreamOutput,
  VolumeOutputMaster,
  MuteOutputMaster
};

using Float = std::conditional_t<mts::config::bits_per_channel == 32, Float32, Float64>;

///
/// An AudioServerPlugIn is a CFPlugIn that is loaded by the host process as a driver. The plug-in
/// bundle is installed in /Library/Audio/Plug-Ins/HAL. The bundle's name has the suffix ".driver".
/// When loading the plug-in, the host looks for factories with the plug-in type,
/// kAudioServerPlugInTypeUUID. The plug-in provides an object that conforms to the interface,
/// kAudioServerPlugInDriverInterfaceUUID.
///
/// An AudioServerPlugIn operates in its own process separate from the system daemon. First and
/// foremost, an AudioServerPlugIn may not make any calls to the client HAL API in the
/// CoreAudio.framework. This will result in undefined (but generally bad) behavior.
///
/// Further, the host process is sandboxed. As such, an AudioServerPlugIn may only read files in its
/// bundle in addition to the system libraries and frameworks. It may not access user documents or
/// write to any filesystem locations other than the system's cache and temporary directories as
/// derived through Apple API. The host provides a means for the plug-in to store and retrieve data
/// from persistent storage.
///
/// An AudioServerPlugIn may communicate with other processes on the system. However, the plug-in
/// must list the name of the mach services to be accessed in the plug-in bundle's info.plist in a
/// key named "AudioServerPlugIn_MachServices". The value of this key is an array of the names of
/// the mach services that need to be accessed.
///
/// An AudioServerPlugIn may also use network resources. However, the plug-in must declare this in
/// its bundle's info.plist with the key named, "AudioServerPlugIn_Network". The value of this key
/// is a boolean and must be set to true if the key is present.
///
/// When the state of an AudioObject implemented by the plug-in changes, it notifies the host using
/// the host routine, PropertiesChanged(). The only exception to this is for AudioDevice objects.
/// AudioDevices may call the host's PropertiesChanged() routine only for state changes that don't
/// have any effect on IO or on the structure of the AudioDevice, such as a change to the value of a
/// volume control.
///
/// For changes to an AudioDevice's state that will affect IO or its structure, the change may not
/// be made without first making a call to the host's RequestDeviceConfigurationChange() routine.
/// This allows the host an opportunity to stop any outstanding IO and otherwise return the device
/// to its ground state. The host will inform the plug-in that it is safe to make the change by
/// calling the plug-in routine, PerformDeviceConfigurationChange(). It is only at this point that
/// the device can make the state change. When PerformDeviceConfigurationChange() returns, the host
/// will figure out what changed and restart any outstanding IO.
///
/// The host is in control of IO. It tells the plug-in's AudioDevice when to start and when to stop
/// the hardware. The host drives its timing using the timestamps provided by the AudioDevice's
/// implementation of GetZeroTimeStamp(). The series of timestamps provides a mapping between the
/// device's sample time and mach_absolute_time().
///
/// The host provides the plug-in's device access to several tap points into the system's mix engine
/// to allow for a variety of features, including adding processing to the signal. The host breaks
/// these tap points down into IO operations that the host asks the plug-in to perform at the
/// appropriate time. Prior to starting IO, the host will ask the plug-in which operations are to be
/// performed. Note that the IO operations are performed on a real time thread on a deadline. As
/// such the plug-in must avoid avoid blocking and return as quickly as possible.
///
class Driver : public AudioServerPlugInDriverInterface {
public:
  static Driver& instance();
  static AudioServerPlugInDriverRef handle();

  ~Driver() = default;

  inline const AudioServerPlugInHostInterface* getPluginHost() const noexcept { return m_pluginHost; }
  inline mts::mutex& getMutex() noexcept { return m_stateMutex; }
  inline bool isBoxAcquired() const noexcept { return m_isBoxAcquired; }
  inline void setBoxAcquired(bool ac) noexcept { m_isBoxAcquired = ac; }
  inline CFStringRef& get_box_name() noexcept { return m_boxName; }
  inline CFStringRef get_box_name() const noexcept { return m_boxName; }
  inline UInt64 getIoRunning() const noexcept { return m_ioRunning; }
  inline Float64 get_sample_rate() const noexcept { return m_sampleRate; }
  inline bool isInputStreamActive() const noexcept { return m_streamInputActive; }
  inline void setInputStreamActive(bool active) noexcept { m_streamInputActive = active; }
  inline bool isOutputStreamActive() const noexcept { return m_streamOutputActive; }
  inline void setOutputStreamActive(bool active) noexcept { m_streamOutputActive = active; }
  inline void setMasterMute(bool muted) noexcept { m_muteMasterValue = muted; }
  inline bool isMasterMuted() const noexcept { return m_muteMasterValue; }
  inline Float32 getMasterVolume() const noexcept { return m_volumeMasterValue; }
  inline void setMasterVolume(Float32 value) noexcept { m_volumeMasterValue = value; }

  inline Float32 getMasterVolumeDecibel() const noexcept {
    return mts::clamp(
        mts::amplitude_to_decibel(m_volumeMasterValue), mts::config::volume_min_db, mts::config::volume_max_db);
  }

  inline Float32 getMasterVolumeNormalized() const noexcept {
    return mts::amplitude_to_normalized_value(
        m_volumeMasterValue, mts::config::volume_min_db, mts::config::volume_max_db);
  }

  template <typename Fct>
  inline void safeCall(Fct&& fct) {
    m_stateMutex.lock();
    fct();
    m_stateMutex.unlock();
  }

private:
  ULONG m_refCount;
  const AudioServerPlugInHostInterface* m_pluginHost = nullptr;
  CFStringRef m_boxName = nullptr;
  bool m_isBoxAcquired = true;
  Float64 m_sampleRate = mts::config::default_sample_rate;
  UInt64 m_ioRunning = 0;
  Float64 m_hostTicksPerFrame = 0.0;
  UInt64 m_numberTimeStamps = 0;
  Float64 m_anchorSampleTime = 0.0;
  UInt64 m_anchorHostTime = 0;
  bool m_streamInputActive = true;
  bool m_streamOutputActive = true;
  Float32 m_volumeMasterValue = 1.0;
  bool m_muteMasterValue = false;
  Float* m_ringBuffer = nullptr;

  // Keep track of last outputSampleTime and the cleared buffer status.
  Float64 m_lastOutputSampleTime = 0;
  Boolean m_isBufferClear = true;

  mts::mutex m_stateMutex;
  mts::mutex m_ioMutex;

  static void initialize();

  Driver();

  HRESULT QueryInterfaceImpl(void* drv, REFIID inUUID, LPVOID* outInterface);
  ULONG AddRefImpl(void* drv);
  ULONG ReleaseImpl(void* drv);
  OSStatus InitializeImpl(AudioServerPlugInHostRef inHost);
  OSStatus CreateDeviceImpl(
      CFDictionaryRef inDescription, const AudioServerPlugInClientInfo* cinfo, AudioObjectID* outDeviceObjectID);
  OSStatus DestroyDeviceImpl(AudioObjectID devID);
  OSStatus AddDeviceClientImpl(AudioObjectID devID, const AudioServerPlugInClientInfo* cinfo);
  OSStatus RemoveDeviceClientImpl(AudioObjectID devID, const AudioServerPlugInClientInfo* cinfo);
  OSStatus PerformDeviceConfigurationChangeImpl(AudioObjectID devID, UInt64 inChangeAction, void* inChangeInfo);
  OSStatus AbortDeviceConfigurationChangeImpl(AudioObjectID devID, UInt64 inChangeAction, void* inChangeInfo);

  Boolean HasPropertyImpl(AudioObjectID objID, pid_t cpid, const AudioObjectPropertyAddress* inAddress);

  OSStatus IsPropertySettableImpl(
      AudioObjectID objID, pid_t cpid, const AudioObjectPropertyAddress* inAddress, Boolean* outIsSettable);

  OSStatus GetPropertyDataSizeImpl(AudioObjectID objID, pid_t cpid, const AudioObjectPropertyAddress* inAddress,
      UInt32 inQualifierDataSize, const void* inQualifierData, UInt32* outDataSize);

  OSStatus GetPropertyDataImpl(AudioObjectID objID, pid_t cpid, const AudioObjectPropertyAddress* inAddress,
      UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, UInt32* outDataSize, void* outData);

  OSStatus SetPropertyDataImpl(AudioObjectID objID, pid_t cpid, const AudioObjectPropertyAddress* inAddress,
      UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData);

  OSStatus StartIOImpl(AudioObjectID devID, UInt32 inClientID);

  OSStatus StopIOImpl(AudioObjectID devID, UInt32 inClientID);

  OSStatus GetZeroTimeStampImpl(
      AudioObjectID devID, UInt32 inClientID, Float64* outSampleTime, UInt64* outHostTime, UInt64* outSeed);

  OSStatus WillDoIOOperationImpl(
      AudioObjectID devID, UInt32 inClientID, UInt32 inOperationID, Boolean* outWillDo, Boolean* outWillDoInPlace);

  OSStatus BeginIOOperationImpl(AudioObjectID devID, UInt32 inClientID, UInt32 inOperationID,
      UInt32 inIOBufferFrameSize, const AudioServerPlugInIOCycleInfo* inIOCycleInfo);

  OSStatus DoIOOperationImpl(AudioObjectID devID, AudioObjectID inStreamObjectID, UInt32 inClientID,
      UInt32 inOperationID, UInt32 inIOBufferFrameSize, const AudioServerPlugInIOCycleInfo* inIOCycleInfo,
      void* ioMainBuffer, void* ioSecondaryBuffer);

  OSStatus EndIOOperationImpl(AudioObjectID devID, UInt32 inClientID, UInt32 inOperationID, UInt32 inIOBufferFrameSize,
      const AudioServerPlugInIOCycleInfo* inIOCycleInfo);
};

static_assert(std::is_trivially_destructible_v<Driver>, "Driver must remain trivially destructable");

// Non-local variables :
//
// All non-local variables with static storage duration are initialized as part of program startup,
// before the execution of the main function begins (unless deferred, see below).
// All non-local variables with thread-local storage duration are initialized as part of thread launch,
// sequenced-before the execution of the thread function begins. For both of these classes of variables,
// initialization occurs in two distinct stages:
//
// Static initialization :
//
// There are two forms of static initialization:
// 1) If relevant, constant initialization is applied.
// 2) Otherwise, non-local static and thread-local variables are zero-initialized.
//
// In practice:
//
// Constant initialization is usually applied at compile time.
// Pre-calculated object representations are stored as part of the program image.
// If the compiler doesn't do that, it must still guarantee that the initialization happens before
// any dynamic initialization.
// Variables to be zero-initialized are placed in the .bss segment of the program image,
// which occupies no space on disk and is zeroed out by the OS when loading the program.
//
// https://en.cppreference.com/w/cpp/language/initialization#Non-local_variables
// https://pabloariasal.github.io/2020/01/02/static-variable-initialization/
static char driverContent[sizeof(Driver)];
static Driver* driverInstance = nullptr;
static Driver** driverInstanceRef = nullptr;

void Driver::initialize() {
  driverInstance = new (driverContent) Driver();
  driverInstanceRef = &driverInstance;
}

Driver& Driver::instance() {
  if (!driverInstance) {
    initialize();
  }

  return *driverInstance;
}

AudioServerPlugInDriverRef Driver::handle() {
  if (!driverInstanceRef) {
    initialize();
  }

  return (AudioServerPlugInDriverRef)driverInstanceRef;
}

inline Driver& driver() { return Driver::instance(); }

inline void async(dispatch_block_t block) {
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), block);
}

///
///
///
class MasterMute : public mts::core::mute_control<MasterMute> {
public:
  inline MasterMute(ObjectID objID, ObjectID deviceID, mts::direction direction)
      : mute_control(static_cast<AudioObjectID>(objID), static_cast<AudioObjectID>(deviceID), direction) {}

  void set_muted(bool muted) const {
    driver().safeCall([=]() { driver().setMasterMute(muted); });
  }

  bool is_muted() const { return driver().isMasterMuted(); }
};

///
///
///
class MasterVolume : public mts::core::volume_control<MasterVolume> {
public:
  inline MasterVolume(ObjectID objID, ObjectID deviceID, mts::direction direction)
      : volume_control(static_cast<AudioObjectID>(objID), static_cast<AudioObjectID>(deviceID), direction) {}

  bool set_volume_normalized(Float32 value) const {
    Float32 volume = mts::normalized_value_to_amplitude(value, mts::config::volume_min_db, mts::config::volume_max_db);

    mts::scoped_lock lock(driver().getMutex());

    if (driver().getMasterVolume() == volume) {
      return false;
    }

    driver().setMasterVolume(volume);
    return true;
  }

  bool set_volume_decibel(Float32 db) const {
    Float32 volume = mts::max(mts::decibel_to_amplitude(db), mts::config::volume_min_amplitude);

    mts::scoped_lock lock(driver().getMutex());
    if (driver().getMasterVolume() == volume) {
      return false;
    }

    driver().setMasterVolume(volume);
    return true;
  }

  Float32 get_volume_decibel() const { return driver().getMasterVolumeDecibel(); }

  Float32 get_volume_normalized() const { return driver().getMasterVolumeNormalized(); }

  Float32 convert_normalized_to_decibel(Float32 value) const {
    // We square the scalar value before converting to dB so as to
    // provide a better curve for the slider.
    value = mts::clamp<Float32>(value, 0.0f, 1.0f);
    return mts::config::volume_min_db + value * value * (mts::config::volume_max_db - mts::config::volume_min_db);
  }

  Float32 convert_decibel_to_normalized(Float32 db) const {
    // We squared the scalar value before converting to dB so we undo that here.
    db = mts::clamp<Float32>(db, mts::config::volume_min_db, mts::config::volume_max_db);
    return sqrtf((db - mts::config::volume_min_db) / (mts::config::volume_max_db - mts::config::volume_min_db));
  }

  AudioValueRange get_volume_decibel_range() const { return mts::config::volume_range_db; }
};

///
///
///
class Box : public mts::core::box<Box> {
public:
  inline Box(ObjectID objID, ObjectID pluginID)
      : mts::core::box<Box>(static_cast<AudioObjectID>(objID), static_cast<AudioObjectID>(pluginID)) {}

  static bool is_acquired() {
    mts::scoped_lock lock(driver().getMutex());
    return driver().isBoxAcquired();
  }

  void save_box_acquired_property() const {
    driver().getPluginHost()->WriteToStorage(driver().getPluginHost(), CFSTR(MTS_PROPERTY_BOX_ACQUIRED),
        driver().isBoxAcquired() ? kCFBooleanTrue : kCFBooleanFalse);
  }

  void save_box_name_property() const {
    if (driver().get_box_name()) {
      driver().getPluginHost()->WriteToStorage(
          driver().getPluginHost(), CFSTR(MTS_PROPERTY_BOX_NAME), driver().get_box_name());
    }
    else {
      driver().getPluginHost()->DeleteFromStorage(driver().getPluginHost(), CFSTR(MTS_PROPERTY_BOX_NAME));
    }
  }

  bool set_acquired(bool acquired) const {
    mts::scoped_lock lock(driver().getMutex());

    if (driver().isBoxAcquired() == acquired) {
      return false;
    }

    driver().setBoxAcquired(acquired);
    save_box_acquired_property();

    // The device list has changed for the plug-in too.
    async(^() {
        AudioObjectPropertyAddress theAddress
            = { kAudioPlugInPropertyDeviceList, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };

        driver().getPluginHost()->PropertiesChanged(
            driver().getPluginHost(), static_cast<AudioObjectID>(get_plugin_id()), 1, &theAddress);
    });

    return true;
  }

  bool set_box_name(CFStringRef name) const {
    mts::scoped_lock lock(driver().getMutex());

    if (!name && !driver().get_box_name()) {
      return false;
    }

    if (!driver().get_box_name()) {
      driver().get_box_name() = name;
      CFRetain(name);
      save_box_name_property();

      return true;
    }

    if (!name) {
      CFRelease(driver().get_box_name());
      driver().get_box_name() = nullptr;
      save_box_name_property();
      return true;
    }

    if (CFStringCompare(name, driver().get_box_name(), kCFCompareCaseInsensitive) == kCFCompareEqualTo) {
      return false;
    }

    CFRelease(driver().get_box_name());
    driver().get_box_name() = name;
    CFRetain(name);
    save_box_name_property();
    return true;
  }

  CFStringRef get_box_name() const {
    mts::scoped_lock lock(driver().getMutex());
    return driver().get_box_name();
  }

  CFStringRef get_box_model_name() const { return CFSTR(MTS_BOX_MODEL_NAME); }
  CFStringRef get_manufacturer_name() const { return CFSTR(MTS_MANUFACTURER_NAME); }
  CFStringRef get_serial_number() const { return CFSTR(MTS_SERIAL_NUMBER); }
  CFStringRef get_firmware_version() const { return CFSTR(MTS_FIRMWARE_VERSION); }
  CFStringRef get_box_uid() const { return CFSTR(MTS_BOX_UID); }
  UInt32 get_device_list_count() const { return 1; }

  UInt32 get_device_list(AudioObjectID* objs, UInt32 itemCount) const {
    if (itemCount >= 1) {
      objs[0] = static_cast<AudioObjectID>(ObjectID::Device);
      itemCount = 1;
    }

    return itemCount;
  }
};

///
///
///
class Device : public mts::core::device<Device> {
public:
  inline Device(ObjectID pluginID)
      : mts::core::device<Device>(static_cast<AudioObjectID>(ObjectID::Device), static_cast<AudioObjectID>(pluginID)) {}

  static constexpr std::array objectsDescription = {
    mts::object_description{
        static_cast<AudioObjectID>(ObjectID::StreamInput), mts::object_type::stream, mts::direction::input },
    mts::object_description{
        static_cast<AudioObjectID>(ObjectID::VolumeInputMaster), mts::object_type::control, mts::direction::input },
    mts::object_description{
        static_cast<AudioObjectID>(ObjectID::MuteInputMaster), mts::object_type::control, mts::direction::input },

    mts::object_description{
        static_cast<AudioObjectID>(ObjectID::StreamOutput), mts::object_type::stream, mts::direction::output },
    mts::object_description{
        static_cast<AudioObjectID>(ObjectID::VolumeOutputMaster), mts::object_type::control, mts::direction::output },
    mts::object_description{
        static_cast<AudioObjectID>(ObjectID::MuteOutputMaster), mts::object_type::control, mts::direction::output },
  };

  bool is_hidden() const { return mts::config::hidden; }

  bool allows_default() const { return mts::config::allows_default_device; }

  Float64 get_sample_rate() const {
    mts::scoped_lock lock(driver().getMutex());
    return driver().get_sample_rate();
  }

  UInt32 get_sample_rate_count() const { return mts::config::supported_sample_rates_count; }

  UInt32 get_sample_rates(AudioValueRange* ranges, UInt32 itemCount) const {
    itemCount = mts::min(itemCount, mts::config::supported_sample_rates_count);

    for (UInt32 i = 0; i < itemCount; i++) {
      ranges[i] = AudioValueRange{ mts::config::supported_sample_rates[i], mts::config::supported_sample_rates[i] };
    }
    return itemCount;
  }

  OSStatus set_sample_rate(Float64 sr) const {
    RETURN_ERROR_IF(!mts::config::is_supported_sample_rate(sr), kAudioHardwareIllegalOperationError,
        "unsupported value for kAudioDevicePropertyNominalSampleRate");

    // make sure that the new value is different than the old value.

    Float64 oldSampleRate;
    driver().safeCall([&]() { oldSampleRate = driver().get_sample_rate(); });

    if (oldSampleRate != sr) {
      // We dispatch this so that the change can happen asynchronously.
      async(^{
          driver().getPluginHost()->RequestDeviceConfigurationChange(
              driver().getPluginHost(), static_cast<AudioObjectID>(ObjectID::Device), (UInt64)sr, nullptr);
      });
    }

    return kAudioHardwareNoError;
  }

  bool is_io_running() const {
    mts::scoped_lock lock(driver().getMutex());
    return driver().getIoRunning() > 0;
  }

  UInt32 get_channel_count() const { return mts::config::channel_count; }
  UInt32 get_ring_buffer_size() const { return mts::config::ring_buffer_size; }
  CFStringRef get_device_name() const { return CFSTR(MTS_DEVICE_NAME); }
  CFStringRef get_manufacturer_name() const { return CFSTR(MTS_MANUFACTURER_NAME); }
  CFStringRef get_device_uid() const { return CFSTR(MTS_DEVICE_UID); }
  CFStringRef get_device_model_uid() const { return CFSTR(MTS_DEVICE_MODEL_UID); }
  CFStringRef get_bundle_id() const { return CFSTR(MTS_PLUGIN_BUNDLE_ID); }
  CFStringRef get_icon_file() const { return CFSTR(MTS_ICON_FILE); }
};

///
///
///
class MasterStream : public mts::core::stream<MasterStream> {
public:
  inline MasterStream(ObjectID objID, ObjectID deviceID, mts::direction direction)
      : stream(static_cast<AudioObjectID>(objID), static_cast<AudioObjectID>(deviceID), direction) {}

  inline bool isInput() const { return get_direction() == mts::direction::input; }

  UInt32 get_sample_rate_count() const { return mts::config::supported_sample_rates_count; }

  bool is_active() const { return isInput() ? driver().isInputStreamActive() : driver().isOutputStreamActive(); }

  bool set_active(bool active) const {
    mts::scoped_lock lock(driver().getMutex());

    if (isInput()) {
      if (driver().isInputStreamActive() == active) {
        return false;
      }

      driver().setInputStreamActive(active);
      return true;
    }

    if (driver().isOutputStreamActive() == active) {
      return false;
    }

    driver().setOutputStreamActive(active);
    return true;
  }

  void get_basic_description(AudioStreamBasicDescription& desc) const {
    driver().safeCall([&]() {
      desc.mSampleRate = driver().get_sample_rate();
      desc.mFormatID = mts::config::format_id;
      desc.mFormatFlags = mts::config::format_flags;
      desc.mBytesPerPacket = mts::config::bytes_per_packet;
      desc.mFramesPerPacket = mts::config::frames_per_packet;
      desc.mBytesPerFrame = mts::config::bytes_per_frame;
      desc.mChannelsPerFrame = mts::config::channel_count;
      desc.mBitsPerChannel = mts::config::bits_per_channel;
    });
  }

  void get_ranged_descriptions(AudioStreamRangedDescription* desc, UInt32 itemCount) const {
    for (UInt32 i = 0; i < itemCount; i++) {
      desc[i].mFormat.mSampleRate = mts::config::supported_sample_rates[i];
      desc[i].mFormat.mFormatID = mts::config::format_id;
      desc[i].mFormat.mFormatFlags = mts::config::format_flags;
      desc[i].mFormat.mBytesPerPacket = mts::config::bytes_per_packet;
      desc[i].mFormat.mFramesPerPacket = mts::config::frames_per_packet;
      desc[i].mFormat.mBytesPerFrame = mts::config::bytes_per_frame;
      desc[i].mFormat.mChannelsPerFrame = mts::config::channel_count;
      desc[i].mFormat.mBitsPerChannel = mts::config::bits_per_channel;
      desc[i].mSampleRateRange.mMinimum = mts::config::supported_sample_rates[i];
      desc[i].mSampleRateRange.mMaximum = mts::config::supported_sample_rates[i];
    }
  }

  OSStatus set_format(const AudioStreamBasicDescription* desc) const {
    RETURN_FORMAT_ERROR_IF(desc->mFormatID != mts::config::format_id);
    RETURN_FORMAT_ERROR_IF(desc->mFormatFlags != mts::config::format_flags);
    RETURN_FORMAT_ERROR_IF(desc->mBytesPerPacket != mts::config::bytes_per_packet);
    RETURN_FORMAT_ERROR_IF(desc->mFramesPerPacket != mts::config::frames_per_packet);
    RETURN_FORMAT_ERROR_IF(desc->mBytesPerFrame != mts::config::bytes_per_frame);
    RETURN_FORMAT_ERROR_IF(desc->mChannelsPerFrame != mts::config::channel_count);
    RETURN_FORMAT_ERROR_IF(desc->mBitsPerChannel != mts::config::bits_per_channel);
    RETURN_ERROR_IF(!mts::config::is_supported_sample_rate(desc->mSampleRate), kAudioHardwareIllegalOperationError,
        "unsupported sample rate in kAudioStreamPropertyVirtualFormat");

    Float64 oldSampleRate;
    driver().safeCall([&]() { oldSampleRate = driver().get_sample_rate(); });

    if (desc->mSampleRate != oldSampleRate) {
      // We dispatch this so that the change can happen asynchronously.
      async(^{
          driver().getPluginHost()->RequestDeviceConfigurationChange(driver().getPluginHost(),
              static_cast<AudioObjectID>(get_device_id()), (UInt64)desc->mSampleRate, nullptr);
      });
    }

    return kAudioHardwareNoError;
  }
};

///
///
///
class Plugin : public mts::core::plugin<Plugin> {
public:
  inline Plugin(ObjectID objID)
      : mts::core::plugin<Plugin>(static_cast<AudioObjectID>(objID)) {}

  CFStringRef get_resource_bundle() const { return CFSTR(""); }

  AudioObjectID get_device_from_uid(CFStringRef uid) const {
    if (CFStringCompare(uid, CFSTR(MTS_DEVICE_UID), 0) == kCFCompareEqualTo) {
      return static_cast<AudioObjectID>(ObjectID::Device);
    }

    return kAudioObjectUnknown;
  }

  AudioObjectID get_box_from_uid(CFStringRef uid) const {
    if (CFStringCompare(uid, CFSTR(MTS_BOX_UID), 0) == kCFCompareEqualTo) {
      return static_cast<AudioObjectID>(ObjectID::Box);
    }

    return kAudioObjectUnknown;
  }

  UInt32 get_device_list_size() const { return driver().isBoxAcquired(); }

  UInt32 get_box_list_size() const { return 1; }

  UInt32 get_object_list_size() const { return 1 + driver().isBoxAcquired(); }

  UInt32 get_device_list(AudioObjectID* objs, UInt32 itemCount) const {
    mts::scoped_lock lock(driver().getMutex());

    if (driver().isBoxAcquired() && itemCount >= 1) {
      objs[0] = static_cast<AudioObjectID>(ObjectID::Device);
      return 1;
    }

    return 0;
  }

  UInt32 get_box_list(AudioObjectID* objs, UInt32 itemCount) const {
    if (itemCount >= 1) {
      itemCount = 1;
      objs[0] = static_cast<AudioObjectID>(ObjectID::Box);
    }

    return itemCount;
  }

  UInt32 get_object_list(AudioObjectID* objs, UInt32 itemCount) const {
    if (itemCount == 0) {
      return 0;
    }

    if (itemCount >= 1) {
      objs[0] = static_cast<AudioObjectID>(ObjectID::Box);
    }

    mts::scoped_lock lock(driver().getMutex());

    if (driver().isBoxAcquired() && itemCount >= 2) {
      objs[1] = static_cast<AudioObjectID>(ObjectID::Device);
      return 2;
    }

    return 1;
  }

  CFStringRef get_manufacturer_name() const { return CFSTR(MTS_MANUFACTURER_NAME); }
};

///
///
///
template <typename R, typename Fct>
inline R callObject(AudioObjectID auid, Fct&& fct, R ret) {
  switch (static_cast<ObjectID>(auid)) {
  case ObjectID::Plugin:
    return fct(Plugin(ObjectID::Plugin));

  case ObjectID::Box:
    return fct(Box(ObjectID::Box, ObjectID::Plugin));

  case ObjectID::Device:
    return fct(Device(ObjectID::Plugin));

  case ObjectID::StreamInput:
    return fct(MasterStream(ObjectID::StreamInput, ObjectID::Device, mts::direction::input));

  case ObjectID::VolumeInputMaster:
    return fct(MasterVolume(ObjectID::VolumeInputMaster, ObjectID::Device, mts::direction::input));

  case ObjectID::MuteInputMaster:
    return fct(MasterMute(ObjectID::MuteInputMaster, ObjectID::Device, mts::direction::input));

  case ObjectID::StreamOutput:
    return fct(MasterStream(ObjectID::StreamOutput, ObjectID::Device, mts::direction::output));

  case ObjectID::VolumeOutputMaster:
    return fct(MasterVolume(ObjectID::VolumeOutputMaster, ObjectID::Device, mts::direction::output));

  case ObjectID::MuteOutputMaster:
    return fct(MasterMute(ObjectID::MuteOutputMaster, ObjectID::Device, mts::direction::output));
  }

  return ret;
}

//
//
//
//
//

Driver::Driver() {
  _reserved = nullptr;

  m_refCount = 0;

  QueryInterface = [](void* drv, REFIID inUUID, LPVOID* outInterface) -> HRESULT {
    if (drv != handle()) {
      return kAudioHardwareBadObjectError;
    }

    if (!outInterface) {
      return kAudioHardwareIllegalOperationError;
    }

    return instance().QueryInterfaceImpl(drv, inUUID, outInterface);
  };

  AddRef = [](void* drv) -> ULONG { return drv == handle() ? instance().AddRefImpl(drv) : 0; };

  Release = [](void* drv) -> ULONG { return drv == handle() ? instance().ReleaseImpl(drv) : 0; };

  HasProperty = [](AudioServerPlugInDriverInterface** drv, auto... args) -> Boolean {
    return drv == handle() ? instance().HasPropertyImpl(args...) : false;
  };

#define FORWARD_FCT(Name)                                                                                              \
  Name = [](AudioServerPlugInDriverInterface** drv, auto... args) -> OSStatus {                                        \
    return drv == handle() ? instance().Name##Impl(args...) : kAudioHardwareBadObjectError;                            \
  }

  FORWARD_FCT(Initialize);
  FORWARD_FCT(CreateDevice);
  FORWARD_FCT(DestroyDevice);
  FORWARD_FCT(AddDeviceClient);
  FORWARD_FCT(RemoveDeviceClient);
  FORWARD_FCT(PerformDeviceConfigurationChange);
  FORWARD_FCT(AbortDeviceConfigurationChange);
  FORWARD_FCT(IsPropertySettable);
  FORWARD_FCT(GetPropertyDataSize);
  FORWARD_FCT(GetPropertyData);
  FORWARD_FCT(SetPropertyData);
  FORWARD_FCT(StartIO);
  FORWARD_FCT(StopIO);
  FORWARD_FCT(GetZeroTimeStamp);
  FORWARD_FCT(WillDoIOOperation);
  FORWARD_FCT(BeginIOOperation);
  FORWARD_FCT(DoIOOperation);
  FORWARD_FCT(EndIOOperation);
#undef FORWARD_FCT
}

// This function is called by the HAL to get the interface to talk to the plug-in through.
// AudioServerPlugIns are required to support the IUnknown interface and the
// AudioServerPlugInDriverInterface. As it happens, all interfaces must also provide the
// IUnknown interface, so we can always just return the single interface we made with
// gAudioServerPlugInDriverInterfacePtr regardless of which one is asked for.
HRESULT Driver::QueryInterfaceImpl(void* inDriver, REFIID iid, LPVOID* outInterface) {
  CFUUIDRef theRequestedUUID = CFUUIDCreateFromUUIDBytes(kCFAllocatorDefault, iid);

  if (!theRequestedUUID) {
    return kAudioHardwareIllegalOperationError;
  }

  // AudioServerPlugIns only support two interfaces, IUnknown (which has to be supported by all
  // CFPlugIns and AudioServerPlugInDriverInterface (which is the actual interface the HAL will
  // use).

  if (!(CFEqual(theRequestedUUID, IUnknownUUID) || CFEqual(theRequestedUUID, kAudioServerPlugInDriverInterfaceUUID))) {
    CFRelease(theRequestedUUID);
    return E_NOINTERFACE;
  }

  safeCall([&]() { m_refCount++; });

  *outInterface = handle();
  CFRelease(theRequestedUUID);
  return S_OK;
}

/// @method     AddRef
/// @abstract   The IUnknown method for retaining a reference to a CFPlugIn type.
/// @param      inDriver The CFPlugIn type to retain.
/// @result     The resulting reference count after the new reference is added.
ULONG Driver::AddRefImpl(void* inDriver) {
  ULONG count = 0;

  {
    mts::scoped_lock lock(m_stateMutex);
    if (m_refCount < UINT32_MAX) {
      ++m_refCount;
    }
    count = m_refCount;
  }

  return count;
}

/// @method     Release
/// @abstract   The IUnknown method for releasing a reference to a CFPlugIn type.
/// @param      inDriver The CFPlugIn type to release.
/// @result     The resulting reference count after the reference has been removed.
ULONG Driver::ReleaseImpl(void* inDriver) {
  ULONG count = 0;

  {
    mts::scoped_lock lock(m_stateMutex);
    if (m_refCount > 0) {
      --m_refCount;
    }
    count = m_refCount;
  }

  return count;
}

inline bool getInitBoxAcquiredProperty(AudioServerPlugInHostRef host) {
  CFPropertyListRef settings = nullptr;

  if (host->CopyFromStorage(host, CFSTR(MTS_PROPERTY_BOX_ACQUIRED), &settings) != kAudioHardwareNoError) {
    return true;
  }

  if (!settings) {
    return true;
  }

  CFTypeID type_id = CFGetTypeID(settings);
  SInt32 result = 1;

  if (type_id == CFBooleanGetTypeID()) {
    result = (SInt32)CFBooleanGetValue((CFBooleanRef)settings);
  }
  else if (type_id == CFNumberGetTypeID()) {
    CFNumberGetValue((CFNumberRef)settings, kCFNumberSInt32Type, &result);
  }

  CFRelease(settings);
  return (bool)result;
}

inline CFStringRef getInitBoxNameProperty(AudioServerPlugInHostRef host) {
  CFPropertyListRef settings = nullptr;

  if (host->CopyFromStorage(host, CFSTR(MTS_PROPERTY_BOX_NAME), &settings) != kAudioHardwareNoError) {
    return CFSTR(MTS_DEFAULT_BOX_NAME);
  }

  if (!settings) {
    return CFSTR(MTS_DEFAULT_BOX_NAME);
  }

  if (CFGetTypeID(settings) != CFStringGetTypeID()) {
    CFRelease(settings);
    return CFSTR(MTS_DEFAULT_BOX_NAME);
  }

  CFStringRef result = (CFStringRef)settings;
  CFRetain(result);
  CFRelease(settings);
  return result;
}

// The job of this method is, as the name implies, to get the driver initialized. One specific
// thing that needs to be done is to store the AudioServerPlugInHostRef so that it can be used
// later. Note that when this call returns, the HAL will scan the various lists the driver
// maintains (such as the device list) to get the inital set of objects the driver is
// publishing. So, there is no need to notifiy the HAL about any objects created as part of the
// execution of this method.

///
/// @method         Initialize
/// @abstract       This method is called to initialize the instance of the plug-in.
/// @discussion     As part of initialization, the plug-in should publish all the objects it
///                 knows about at the time.
///
/// @param          inDriver
///                        The plug-in to initialize.
/// @param          inHost
///                        An AudioServerPlugInHostInterface struct that the plug-in is to use for
///                        communication with the Host. The Host guarantees that the storage
///                        pointed to by inHost will remain valid for the lifetime of the plug-in.
/// @result         An OSStatus indicating success or failure.
///
OSStatus Driver::InitializeImpl(AudioServerPlugInHostRef inHost) {
  m_pluginHost = inHost;

  // Initialize the box acquired property from the settings.
  m_isBoxAcquired = getInitBoxAcquiredProperty(m_pluginHost);

  // Initialize box name from the settings.
  m_boxName = getInitBoxNameProperty(m_pluginHost);

  // Calculate the host ticks per frame.
  struct mach_timebase_info theTimeBaseInfo;
  mach_timebase_info(&theTimeBaseInfo);
  Float64 theHostClockFrequency = ((Float64)theTimeBaseInfo.denom / (Float64)theTimeBaseInfo.numer) * 1000000000.0;
  m_hostTicksPerFrame = theHostClockFrequency / m_sampleRate;

  return kAudioHardwareNoError;
}

// This method is used to tell a driver that implements the Transport Manager semantics to
// create an AudioEndpointDevice from a set of AudioEndpoints. Since this driver is not a
// Transport Manager, we just check the arguments and return
// kAudioHardwareUnsupportedOperationError.
OSStatus Driver::CreateDeviceImpl(
    CFDictionaryRef inDescription, const AudioServerPlugInClientInfo* inClientInfo, AudioObjectID* outDeviceObjectID) {
  return kAudioHardwareUnsupportedOperationError;
}

// This method is used to tell a driver that implements the Transport Manager semantics to
// destroy an AudioEndpointDevice. Since this driver is not a Transport Manager, we just check
// the arguments and return kAudioHardwareUnsupportedOperationError.
OSStatus Driver::DestroyDeviceImpl(AudioObjectID inDeviceObjectID) { return kAudioHardwareUnsupportedOperationError; }

// This method is used to inform the driver about a new client that is using the given device.
// This allows the device to act differently depending on who the client is. This driver does
// not need to track the clients using the device, so we just check the arguments and return
// successfully.
OSStatus Driver::AddDeviceClientImpl(AudioObjectID inDeviceObjectID, const AudioServerPlugInClientInfo* inClientInfo) {
  if (static_cast<ObjectID>(inDeviceObjectID) != ObjectID::Device) {
    return kAudioHardwareBadObjectError;
  }

  return kAudioHardwareNoError;
}

OSStatus Driver::RemoveDeviceClientImpl(
    AudioObjectID inDeviceObjectID, const AudioServerPlugInClientInfo* inClientInfo) {
  if (static_cast<ObjectID>(inDeviceObjectID) != ObjectID::Device) {
    return kAudioHardwareBadObjectError;
  }

  return kAudioHardwareNoError;
}

// This method is called to tell the device that it can perform the configuation change that it
// had requested via a call to the host method, RequestDeviceConfigurationChange(). The
// arguments, inChangeAction and inChangeInfo are the same as what was passed to
// RequestDeviceConfigurationChange().
//
// The HAL guarantees that IO will be stopped while this method is in progress. The HAL will
// also handle figuring out exactly what changed for the non-control related properties. This
// means that the only notifications that would need to be sent here would be for either
// custom properties the HAL doesn't know about or for controls.
//
// For the device implemented by this driver, only sample rate changes go through this process
// as it is the only state that can be changed for the device that isn't a control. For this
// change, the new sample rate is passed in the inChangeAction argument.
OSStatus Driver::PerformDeviceConfigurationChangeImpl(
    AudioObjectID inDeviceObjectID, UInt64 inChangeAction, void* inChangeInfo) {
  RETURN_ERROR_IF(
      inDeviceObjectID != static_cast<AudioObjectID>(ObjectID::Device), kAudioHardwareBadObjectError, "Bad device ID");
  RETURN_ERROR_IF(
      !mts::config::is_supported_sample_rate((Float64)inChangeAction), kAudioHardwareBadObjectError, "Bad sample rate");

  mts::scoped_lock lock(m_stateMutex);

  // Set sample rate.
  m_sampleRate = inChangeAction;

  // Recalculate the state that depends on the sample rate.
  struct mach_timebase_info theTimeBaseInfo;
  mach_timebase_info(&theTimeBaseInfo);
  Float64 theHostClockFrequency = ((Float64)theTimeBaseInfo.denom / (Float64)theTimeBaseInfo.numer) * 1000000000.0;
  m_hostTicksPerFrame = theHostClockFrequency / m_sampleRate;

  return kAudioHardwareNoError;
}

// This method is called to tell the driver that a request for a config change has been denied.
// This provides the driver an opportunity to clean up any state associated with the request.
// For this driver, an aborted config change requires no action. So we just check the arguments
// and return
OSStatus Driver::AbortDeviceConfigurationChangeImpl(
    AudioObjectID inDeviceObjectID, UInt64 inChangeAction, void* inChangeInfo) {
  RETURN_ERROR_IF(
      inDeviceObjectID != static_cast<AudioObjectID>(ObjectID::Device), kAudioHardwareBadObjectError, "Bad device ID");
  return kAudioHardwareNoError;
}

// This method returns whether or not the given object has the given property.
Boolean Driver::HasPropertyImpl(
    AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress) {
  if (!inAddress) {
    return false;
  }

  return callObject<bool>(
      inObjectID, [=](const auto& obj) { return obj.exists(inAddress); }, false);
}

// This method returns whether or not the given property on the object can have its value changed.
OSStatus Driver::IsPropertySettableImpl(AudioObjectID inObjectID, pid_t inClientProcessID,
    const AudioObjectPropertyAddress* inAddress, Boolean* outIsSettable) {
  if (!inAddress) {
    return kAudioHardwareIllegalOperationError;
  }

  if (!outIsSettable) {
    return kAudioHardwareIllegalOperationError;
  }

  return callObject<OSStatus>(
      inObjectID, [=](const auto& obj) { return obj.is_settable(inAddress, outIsSettable); },
      kAudioHardwareBadObjectError);
}

// This method returns the byte size of the property's data.
OSStatus Driver::GetPropertyDataSizeImpl(AudioObjectID inObjectID, pid_t inClientProcessID,
    const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData,
    UInt32* outDataSize) {
  if (!inAddress) {
    return kAudioHardwareIllegalOperationError;
  }

  if (!outDataSize) {
    return kAudioHardwareIllegalOperationError;
  }

  return callObject<OSStatus>(
      inObjectID,
      [=](const auto& obj) { return obj.size(inAddress, inQualifierDataSize, inQualifierData, outDataSize); },
      kAudioHardwareBadObjectError);
}

OSStatus Driver::GetPropertyDataImpl(AudioObjectID inObjectID, pid_t inClientProcessID,
    const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData,
    UInt32 inDataSize, UInt32* outDataSize, void* outData) {
  if (!inAddress) {
    return kAudioHardwareIllegalOperationError;
  }

  if (!outDataSize) {
    return kAudioHardwareIllegalOperationError;
  }

  if (!outData) {
    return kAudioHardwareIllegalOperationError;
  }

  return callObject<OSStatus>(
      inObjectID,
      [=](const auto& obj) {
        return obj.get(inAddress, inQualifierDataSize, inQualifierData, inDataSize, outDataSize, outData);
      },
      kAudioHardwareBadObjectError);
}

OSStatus Driver::SetPropertyDataImpl(AudioObjectID inObjectID, pid_t inClientProcessID,
    const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData,
    UInt32 inDataSize, const void* inData) {
  if (!inAddress) {
    return kAudioHardwareIllegalOperationError;
  }

  UInt32 theNumberPropertiesChanged = 0;
  AudioObjectPropertyAddress theChangedAddresses[2];

  OSStatus status = callObject<OSStatus>(
      inObjectID,
      [=, &theNumberPropertiesChanged, &theChangedAddresses](const auto& obj) {
        return obj.set(inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData, &theNumberPropertiesChanged,
            theChangedAddresses);
      },
      kAudioHardwareBadObjectError);

  if (theNumberPropertiesChanged > 0) {
    m_pluginHost->PropertiesChanged(m_pluginHost, inObjectID, theNumberPropertiesChanged, theChangedAddresses);
  }

  return status;
}

// This call tells the device that IO is starting for the given client. When this routine
// returns, the device's clock is running and it is ready to have data read/written. It is
// important to note that multiple clients can have IO running on the device at the same time.
// So, work only needs to be done when the first client starts. All subsequent starts simply
// increment the counter.
OSStatus Driver::StartIOImpl(AudioObjectID inDeviceObjectID, UInt32 inClientID) {
  RETURN_ERROR_IF(
      inDeviceObjectID != static_cast<AudioObjectID>(ObjectID::Device), kAudioHardwareBadObjectError, "Bad device ID");

  mts::scoped_lock lock(m_stateMutex);

  if (m_ioRunning == UINT64_MAX) {
    return kAudioHardwareIllegalOperationError;
  }

  if (m_ioRunning == 0) {
    // We need to start the hardware, which in this case is just anchoring the time line.
    m_ioRunning = 1;
    m_numberTimeStamps = 0;
    m_anchorSampleTime = 0;
    m_anchorHostTime = mach_absolute_time();

    // Allocate ring buffer.
    m_ringBuffer = (Float*)calloc(mts::config::ring_buffer_frame_size * mts::config::channel_count, sizeof(Float));

    return kAudioHardwareNoError;
  }

  // IO is already running, so just bump the counter
  ++m_ioRunning;

  return kAudioHardwareNoError;
}

// This call tells the device that the client has stopped IO. The driver can stop the hardware
// once all clients have stopped.
OSStatus Driver::StopIOImpl(AudioObjectID inDeviceObjectID, UInt32 inClientID) {
  RETURN_ERROR_IF(
      inDeviceObjectID != static_cast<AudioObjectID>(ObjectID::Device), kAudioHardwareBadObjectError, "Bad device ID");

  mts::scoped_lock lock(m_stateMutex);

  if (m_ioRunning == 0) {
    return kAudioHardwareIllegalOperationError;
  }

  if (m_ioRunning == 1) {
    // We need to stop the hardware, which in this case means that there's nothing to do.
    m_ioRunning = 0;
    free(m_ringBuffer);
    m_ringBuffer = nullptr;
    return kAudioHardwareNoError;
  }

  --m_ioRunning;

  return kAudioHardwareNoError;
}

// This method returns the current zero time stamp for the device. The HAL models the timing of
// a device as a series of time stamps that relate the sample time to a host time. The zero
// time stamps are spaced such that the sample times are the value of
// kAudioDevicePropertyZeroTimeStampPeriod apart. This is often modeled using a ring buffer
// where the zero time stamp is updated when wrapping around the ring buffer.
//
// For this device, the zero time stamps' sample time increments every kDevice_RingBufferSize
// frames and the host time increments by kDevice_RingBufferSize * gDevice_HostTicksPerFrame.
OSStatus Driver::GetZeroTimeStampImpl(
    AudioObjectID inDeviceObjectID, UInt32 inClientID, Float64* outSampleTime, UInt64* outHostTime, UInt64* outSeed) {
  RETURN_ERROR_IF(
      inDeviceObjectID != static_cast<AudioObjectID>(ObjectID::Device), kAudioHardwareBadObjectError, "Bad device ID");

  mts::scoped_lock lock(m_ioMutex);

  // Get the current host time.
  UInt64 currentHostTime = mach_absolute_time();

  // Calculate the next host time.
  Float64 hostTicksPerRingBuffer = m_hostTicksPerFrame * ((Float64)mts::config::ring_buffer_size);

  Float64 hostTickOffset = ((Float64)(m_numberTimeStamps + 1)) * hostTicksPerRingBuffer;

  UInt64 nextHostTime = m_anchorHostTime + ((UInt64)hostTickOffset);

  // Go to the next time if the next host time is less than the current time.
  if (nextHostTime <= currentHostTime) {
    ++m_numberTimeStamps;
  }

  // Set the return values.
  *outSampleTime = m_numberTimeStamps * mts::config::ring_buffer_size;
  *outHostTime = m_anchorHostTime + (((Float64)m_numberTimeStamps) * hostTicksPerRingBuffer);
  *outSeed = 1;

  return kAudioHardwareNoError;
}

// This method returns whether or not the device will do a given IO operation. For this device,
// we only support reading input data and writing output data.
OSStatus Driver::WillDoIOOperationImpl(AudioObjectID inDeviceObjectID, UInt32 inClientID, UInt32 inOperationID,
    Boolean* outWillDo, Boolean* outWillDoInPlace) {
  RETURN_ERROR_IF(
      inDeviceObjectID != static_cast<AudioObjectID>(ObjectID::Device), kAudioHardwareBadObjectError, "Bad device ID");

  // Figure out if we support the operation.
  bool willDo = false;
  bool willDoInPlace = true;

  switch (inOperationID) {
  case kAudioServerPlugInIOOperationReadInput:
  case kAudioServerPlugInIOOperationWriteMix:
    willDo = true;
    willDoInPlace = true;
  }

  // Set return values.
  if (outWillDo) {
    *outWillDo = willDo;
  }

  if (outWillDoInPlace) {
    *outWillDoInPlace = willDoInPlace;
  }

  return kAudioHardwareNoError;
}

// This is called at the beginning of an IO operation. This device doesn't do anything, so just
// check the arguments and return.
OSStatus Driver::BeginIOOperationImpl(AudioObjectID inDeviceObjectID, UInt32 inClientID, UInt32 inOperationID,
    UInt32 inIOBufferFrameSize, const AudioServerPlugInIOCycleInfo* inIOCycleInfo) {
  RETURN_ERROR_IF(
      inDeviceObjectID != static_cast<AudioObjectID>(ObjectID::Device), kAudioHardwareBadObjectError, "Bad device ID");

  return kAudioHardwareNoError;
}

// This is called to actually perform a given operation.
OSStatus Driver::DoIOOperationImpl(AudioObjectID inDeviceObjectID, AudioObjectID inStreamObjectID, UInt32 inClientID,
    UInt32 inOperationID, UInt32 inIOBufferFrameSize, const AudioServerPlugInIOCycleInfo* inIOCycleInfo,
    void* ioMainBuffer, void* ioSecondaryBuffer) {
  if (inDeviceObjectID != static_cast<AudioObjectID>(ObjectID::Device)) {
    return kAudioHardwareBadObjectError;
  }

  if (!mts::is_one_of(static_cast<ObjectID>(inStreamObjectID), ObjectID::StreamInput, ObjectID::StreamOutput)) {
    return kAudioHardwareBadObjectError;
  }

  if (!mts::is_one_of(inOperationID, kAudioServerPlugInIOOperationReadInput, kAudioServerPlugInIOOperationWriteMix)) {
    return kAudioHardwareNoError;
  }

  const bool isReading = inOperationID == kAudioServerPlugInIOOperationReadInput;

  // Calculate the ring buffer offsets and splits.
  UInt64 mSampleTime = isReading ? inIOCycleInfo->mInputTime.mSampleTime : inIOCycleInfo->mOutputTime.mSampleTime;

  // 'mSampleTime % mts::config::ringBufferFrameSize' == 'mSampleTime & mts::config::ringBufferFrameMask'
  // since mts::config::ringBufferFrameSize is a power of 2.
  UInt32 ringBufferFrameLocationStart = mSampleTime & mts::config::ring_buffer_frame_mask;
  UInt32 firstPartFrameSize = mts::config::ring_buffer_frame_size - ringBufferFrameLocationStart;
  UInt32 secondPartFrameSize = 0;

  if (firstPartFrameSize >= inIOBufferFrameSize) {
    firstPartFrameSize = inIOBufferFrameSize;
  }
  else {
    secondPartFrameSize = inIOBufferFrameSize - firstPartFrameSize;
  }

  // From driver to application.
  if (isReading) {
    Float* outputBuffer = (Float*)ioMainBuffer;

    // If mute is one let's just fill the buffer with zeros or if there's no apps outputing audio.
    if (m_muteMasterValue || (m_lastOutputSampleTime - inIOBufferFrameSize < inIOCycleInfo->mInputTime.mSampleTime)) {
      // Clear the outputBuffer
      mts::dsp::clear(outputBuffer, inIOBufferFrameSize * mts::config::channel_count);

      // Clear the ring buffer.
      if (!m_isBufferClear) {
        // TODO: There is probably a better way than clearing this buffer everytime.
        mts::dsp::clear(m_ringBuffer, mts::config::ring_buffer_frame_size * mts::config::channel_count);
        m_isBufferClear = true;
      }
    }
    else {
      // Copy the buffers.
      mts::dsp::copy(m_ringBuffer + ringBufferFrameLocationStart * mts::config::channel_count, // Src.
          outputBuffer, // Dst.
          firstPartFrameSize * mts::config::channel_count);

      mts::dsp::copy(m_ringBuffer, // Src.
          outputBuffer + firstPartFrameSize * mts::config::channel_count, // Dst.
          secondPartFrameSize * mts::config::channel_count);

      // Finally we'll apply the output volume to the buffer.
      mts::dsp::mul(outputBuffer, (Float)m_volumeMasterValue, inIOBufferFrameSize * mts::config::channel_count);
    }
  }

  // From application to driver.
  else {
    // Save the last output time.
    m_lastOutputSampleTime = inIOCycleInfo->mOutputTime.mSampleTime;
    m_isBufferClear = false;

    const Float* inputBuffer = (const Float*)ioMainBuffer;

    mts::dsp::copy(inputBuffer, // Src.
        m_ringBuffer + ringBufferFrameLocationStart * mts::config::channel_count, // Dst.
        firstPartFrameSize * mts::config::channel_count);

    mts::dsp::copy(inputBuffer + firstPartFrameSize * mts::config::channel_count, // Src.
        m_ringBuffer, // Dst.
        secondPartFrameSize * mts::config::channel_count);
  }

  return kAudioHardwareNoError;
}

// This is called at the end of an IO operation. This device doesn't do anything, so just check
// the arguments and return.
OSStatus Driver::EndIOOperationImpl(AudioObjectID inDeviceObjectID, UInt32 inClientID, UInt32 inOperationID,
    UInt32 inIOBufferFrameSize, const AudioServerPlugInIOCycleInfo* inIOCycleInfo) {
  RETURN_ERROR_IF(
      inDeviceObjectID != static_cast<AudioObjectID>(ObjectID::Device), kAudioHardwareBadObjectError, "Bad device ID");
  return kAudioHardwareNoError;
}

// This is the CFPlugIn factory function. Its job is to create the implementation for the given
// type provided that the type is supported. Because this driver is simple and all its
// initialization is handled via static iniitalization when the bundle is loaded, all that
// needs to be done is to return the AudioServerPlugInDriverRef that points to the driver's
// interface.
extern "C" MTS_API void* MTS_DRIVER_CREATE_PLUGIN(CFAllocatorRef inAllocator, CFUUIDRef inRequestedTypeUUID) {
  if (CFEqual(inRequestedTypeUUID, kAudioServerPlugInTypeUUID)) {
    return Driver::handle();
  }

  return nullptr;
}
