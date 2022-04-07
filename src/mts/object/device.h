#pragma once
#include "mts/common.h"

namespace mts::core {
/// @class device
///
/// Interface:
/// @code
///     static constexpr std::array objectsDescription;
///     bool is_hidden() const;
///     bool allows_default() const;
///     Float64 get_sample_rate() const;
///     UInt32 get_sample_rate_count() const;
///     UInt32 get_sample_rates(AudioValueRange* ranges, UInt32 itemCount) const;
///     OSStatus set_sample_rate(Float64 sr) const;
///     UInt32 get_channel_count() const;
///     bool is_io_running() const;
///     UInt32 get_ring_buffer_size() const;
///     CFStringRef get_device_name() const;
///     CFStringRef get_manufacturer_name() const;
///     CFStringRef get_device_uid() const;
///     CFStringRef get_device_model_uid() const;
///     CFStringRef get_bundle_id() const;
///     CFStringRef get_icon_file() const;
/// @endcode
///
template <typename ImplObject>
class device : public mts::object {
public:
  inline device(AudioObjectID objID, AudioObjectID pluginID)
      : object(objID)
      , m_plugin(pluginID) {}

  inline AudioObjectID get_plugin_id() const { return m_plugin; }

  inline bool exists(const Address* inAddress) const {
    switch (inAddress->mSelector) {
    case kAudioObjectPropertyBaseClass:
    case kAudioObjectPropertyClass:
    case kAudioObjectPropertyOwner:
    case kAudioObjectPropertyName:
    case kAudioObjectPropertyManufacturer:
    case kAudioObjectPropertyOwnedObjects:
    case kAudioDevicePropertyDeviceUID:
    case kAudioDevicePropertyModelUID:
    case kAudioDevicePropertyTransportType:
    case kAudioDevicePropertyRelatedDevices:
    case kAudioDevicePropertyClockDomain:
    case kAudioDevicePropertyDeviceIsAlive:
    case kAudioDevicePropertyDeviceIsRunning:
    case kAudioObjectPropertyControlList:
    case kAudioDevicePropertyNominalSampleRate:
    case kAudioDevicePropertyAvailableNominalSampleRates:
    case kAudioDevicePropertyIsHidden:
    case kAudioDevicePropertyZeroTimeStampPeriod:
    case kAudioDevicePropertyIcon:
    case kAudioDevicePropertyStreams:
      return true;

    case kAudioDevicePropertyDeviceCanBeDefaultDevice:
    case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice:
    case kAudioDevicePropertyLatency:
    case kAudioDevicePropertySafetyOffset:
    case kAudioDevicePropertyPreferredChannelsForStereo:
    case kAudioDevicePropertyPreferredChannelLayout:
      return mts::is_one_of(inAddress->mScope, kAudioObjectPropertyScopeInput, kAudioObjectPropertyScopeOutput);
    }

    return false;
  }

  inline OSStatus is_settable(const Address* inAddress, Boolean* outIsSettable) const {
    switch (inAddress->mSelector) {
    case kAudioObjectPropertyBaseClass:
    case kAudioObjectPropertyClass:
    case kAudioObjectPropertyOwner:
    case kAudioObjectPropertyName:
    case kAudioObjectPropertyManufacturer:
    case kAudioObjectPropertyOwnedObjects:
    case kAudioDevicePropertyDeviceUID:
    case kAudioDevicePropertyModelUID:
    case kAudioDevicePropertyTransportType:
    case kAudioDevicePropertyRelatedDevices:
    case kAudioDevicePropertyClockDomain:
    case kAudioDevicePropertyDeviceIsAlive:
    case kAudioDevicePropertyDeviceIsRunning:
    case kAudioDevicePropertyDeviceCanBeDefaultDevice:
    case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice:
    case kAudioDevicePropertyLatency:
    case kAudioDevicePropertyStreams:
    case kAudioObjectPropertyControlList:
    case kAudioDevicePropertySafetyOffset:
    case kAudioDevicePropertyAvailableNominalSampleRates:
    case kAudioDevicePropertyIsHidden:
    case kAudioDevicePropertyPreferredChannelsForStereo:
    case kAudioDevicePropertyPreferredChannelLayout:
    case kAudioDevicePropertyZeroTimeStampPeriod:
    case kAudioDevicePropertyIcon:
      *outIsSettable = false;
      break;

    case kAudioDevicePropertyNominalSampleRate:
      *outIsSettable = true;
      break;

    default:
      return kAudioHardwareUnknownPropertyError;
    }

    return kAudioHardwareNoError;
  }

  inline OSStatus size(
      const Address* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32* outDataSize) const {
    switch (inAddress->mSelector) {
    case kAudioObjectPropertyBaseClass:
      *outDataSize = sizeof(AudioClassID);
      break;

    case kAudioObjectPropertyClass:
      *outDataSize = sizeof(AudioClassID);
      break;

    case kAudioObjectPropertyOwner:
      *outDataSize = sizeof(AudioObjectID);
      break;

    case kAudioObjectPropertyName:
      *outDataSize = sizeof(CFStringRef);
      break;

    case kAudioObjectPropertyManufacturer:
      *outDataSize = sizeof(CFStringRef);
      break;

    case kAudioObjectPropertyOwnedObjects:
      switch (inAddress->mScope) {
      case kAudioObjectPropertyScopeGlobal:
        *outDataSize = get_global_object_list_size() * sizeof(AudioObjectID);
        break;

      case kAudioObjectPropertyScopeInput:
        *outDataSize = get_input_object_list_size() * sizeof(AudioObjectID);
        break;

      case kAudioObjectPropertyScopeOutput:
        *outDataSize = get_output_object_list_size() * sizeof(AudioObjectID);
        break;
      }
      break;

    case kAudioDevicePropertyDeviceUID:
      *outDataSize = sizeof(CFStringRef);
      break;

    case kAudioDevicePropertyModelUID:
      *outDataSize = sizeof(CFStringRef);
      break;

    case kAudioDevicePropertyTransportType:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioDevicePropertyRelatedDevices:
      *outDataSize = sizeof(AudioObjectID);
      break;

    case kAudioDevicePropertyClockDomain:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioDevicePropertyDeviceIsAlive:
      *outDataSize = sizeof(AudioClassID);
      break;

    case kAudioDevicePropertyDeviceIsRunning:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioDevicePropertyDeviceCanBeDefaultDevice:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioDevicePropertyLatency:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioDevicePropertyStreams:
      switch (inAddress->mScope) {
      case kAudioObjectPropertyScopeGlobal:
        *outDataSize = get_global_stream_list_size() * sizeof(AudioObjectID);
        break;

      case kAudioObjectPropertyScopeInput:
        *outDataSize = get_input_stream_list_size() * sizeof(AudioObjectID);
        break;

      case kAudioObjectPropertyScopeOutput:
        *outDataSize = get_output_stream_list_size() * sizeof(AudioObjectID);
        break;
      }
      break;

    case kAudioObjectPropertyControlList: {
      // TODO: Scope ?
      *outDataSize = get_control_list_size() * sizeof(AudioObjectID);
    } break;

    case kAudioDevicePropertySafetyOffset:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioDevicePropertyNominalSampleRate:
      *outDataSize = sizeof(Float64);
      break;

    case kAudioDevicePropertyAvailableNominalSampleRates:
      *outDataSize = get_sample_rate_count() * sizeof(AudioValueRange);
      break;

    case kAudioDevicePropertyIsHidden:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioDevicePropertyPreferredChannelsForStereo:
      *outDataSize = 2 * sizeof(UInt32);
      break;

    case kAudioDevicePropertyPreferredChannelLayout:
      *outDataSize = offsetof(AudioChannelLayout, mChannelDescriptions)
          + (get_channel_count() * sizeof(AudioChannelDescription));
      break;

    case kAudioDevicePropertyZeroTimeStampPeriod:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioDevicePropertyIcon:
      *outDataSize = sizeof(CFURLRef);
      break;

    default:
      return kAudioHardwareUnknownPropertyError;
    }

    return kAudioHardwareNoError;
  }

  inline OSStatus get(const Address* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData,
      UInt32 inDataSize, UInt32* outDataSize, void* outData) const {

    switch (inAddress->mSelector) {
    // The base class for kAudioDeviceClassID is kAudioObjectClassID.
    case kAudioObjectPropertyBaseClass: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioClassID));
      *outDataSize = mts::assign<AudioClassID>(outData, kAudioObjectClassID);
    } break;

    // The class is always kAudioDeviceClassID for devices created by drivers.
    case kAudioObjectPropertyClass: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioClassID));
      *outDataSize = mts::assign<AudioClassID>(outData, kAudioDeviceClassID);
    } break;

    // The device's owner is the plug-in object
    case kAudioObjectPropertyOwner: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioObjectID));
      *outDataSize = mts::assign<AudioObjectID>(outData, m_plugin);
    } break;

    // This is the human readable name of the device.
    case kAudioObjectPropertyName: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(CFStringRef));
      *outDataSize = mts::assign<CFStringRef>(outData, get_device_name());
    } break;

    // This is the human readable name of the maker of the plug-in.
    case kAudioObjectPropertyManufacturer: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(CFStringRef));
      *outDataSize = mts::assign<CFStringRef>(outData, get_manufacturer_name());
    } break;

    // Calculate the number of items that have been requested. Note that this
    // number is allowed to be smaller than the actual size of the list. In such
    // case, only that number of items will be returned
    case kAudioObjectPropertyOwnedObjects: {
      UInt32 itemCount = inDataSize / sizeof(AudioObjectID);

      // The device owns its streams and controls. Note that what is returned here
      // depends on the scope requested.
      switch (inAddress->mScope)
      case kAudioObjectPropertyScopeGlobal: {
        itemCount = get_global_object_list((AudioObjectID*)outData, itemCount);
        break;

      // Input scope means just the objects on the input side.
      case kAudioObjectPropertyScopeInput:
        itemCount = get_input_object_list((AudioObjectID*)outData, itemCount);
        break;

      // Output scope means just the objects on the output side
      case kAudioObjectPropertyScopeOutput:
        itemCount = get_output_object_list((AudioObjectID*)outData, itemCount);
        break;
      }

        *outDataSize = itemCount * sizeof(AudioObjectID);
    } break;

    // This is a CFString that is a persistent token that can identify the same
    // audio device across boot sessions. Note that two instances of the same
    // device must have different values for this property.
    case kAudioDevicePropertyDeviceUID: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(CFStringRef));
      *outDataSize = mts::assign<CFStringRef>(outData, get_device_uid());
    } break;

    // This is a CFString that is a persistent token that can identify audio
    // devices that are the same kind of device. Note that two instances of the
    // save device must have the same value for this property.
    case kAudioDevicePropertyModelUID: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(CFStringRef));
      *outDataSize = mts::assign<CFStringRef>(outData, get_device_model_uid());
    } break;

    // This value represents how the device is attached to the system. This can be
    // any 32 bit integer, but common values for this property are defined in
    // <CoreAudio/AudioHardwareBase.h>
    case kAudioDevicePropertyTransportType: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, kAudioDeviceTransportTypeVirtual);
    } break;

    // The related devices property identifys device objects that are very closely
    // related. Generally, this is for relating devices that are packaged together
    // in the hardware such as when the input side and the output side of a piece
    // of hardware can be clocked separately and therefore need to be represented
    // as separate AudioDevice objects. In such case, both devices would report
    // that they are related to each other. Note that at minimum, a device is
    // related to itself, so this list will always be at least one item long.
    case kAudioDevicePropertyRelatedDevices: {
      // Calculate the number of items that have been requested.
      // We only have the one device.
      UInt32 itemCount = mts::min(inDataSize / sizeof(AudioObjectID), 1);

      // Write the devices' object IDs into the return value
      if (itemCount > 0) {
        ((AudioObjectID*)outData)[0] = get_id();
      }

      *outDataSize = itemCount * sizeof(AudioObjectID);
    } break;

    // This property allows the device to declare what other devices it is
    // synchronized with in hardware. The way it works is that if two devices have
    // the same value for this property and the value is not zero, then the two
    // devices are synchronized in hardware. Note that a device that either can't
    // be synchronized with others or doesn't know should return 0 for this
    // property.
    case kAudioDevicePropertyClockDomain: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, 0);
    } break;

      // This property returns whether or not the device is alive. Note that it is
      // note uncommon for a device to be dead but still momentarily availble in the
      // device list. In the case of this device, it will always be alive.
    case kAudioDevicePropertyDeviceIsAlive: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, 1);
    } break;

    // This property returns whether or not IO is running for the device. Note that
    // we need to take both the state lock to check this value for thread safety.
    case kAudioDevicePropertyDeviceIsRunning: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, is_io_running());
    } break;

    // This property returns whether or not the device wants to be able to be the
    // default device for content. This is the device that iTunes and QuickTime
    // will use to play their content on and FaceTime will use as it's microhphone.
    // Nearly all devices should allow for this.
    case kAudioDevicePropertyDeviceCanBeDefaultDevice: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, allows_default());
    } break;

    // This property returns whether or not the device wants to be the system
    // default device. This is the device that is used to play interface sounds and
    // other incidental or UI-related sounds on. Most devices should allow this
    // although devices with lots of latency may not want to.
    case kAudioDevicePropertyDeviceCanBeDefaultSystemDevice: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, allows_default());
    } break;

    // This property returns the presentation latency of the device. For this,
    // device, the value is 0 due to the fact that it always vends silence.
    case kAudioDevicePropertyLatency: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, 0);
    } break;

    // Calculate the number of items that have been requested. Note that this
    // number is allowed to be smaller than the actual size of the list. In such
    // case, only that number of items will be returned
    case kAudioDevicePropertyStreams: {
      UInt32 itemCount = inDataSize / sizeof(AudioObjectID);

      // Note that what is returned here depends on the scope requested.
      switch (inAddress->mScope) {
      // Global scope means return all streams.
      case kAudioObjectPropertyScopeGlobal:
        itemCount = get_global_stream_list((AudioObjectID*)outData, itemCount);
        break;

      // Input streams.
      case kAudioObjectPropertyScopeInput:
        itemCount = get_input_stream_list((AudioObjectID*)outData, itemCount);
        break;

      // Output streams.
      case kAudioObjectPropertyScopeOutput:
        itemCount = get_output_stream_list((AudioObjectID*)outData, itemCount);
        break;
      }

      *outDataSize = itemCount * sizeof(AudioObjectID);
    } break;

    // Calculate the number of items that have been requested. Note that this
    // number is allowed to be smaller than the actual size of the list. In such
    // case, only that number of items will be returned
    case kAudioObjectPropertyControlList: {
      // TODO: Scope ?
      UInt32 itemCount = inDataSize / sizeof(AudioObjectID);
      itemCount = get_control_list((AudioObjectID*)outData, itemCount);
      *outDataSize = itemCount * sizeof(AudioObjectID);
    } break;

    // This property returns the how close to now the HAL can read and write. For
    // this, device, the value is 0 due to the fact that it always vends silence.
    case kAudioDevicePropertySafetyOffset: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, 0);
    } break;

    // This property returns the nominal sample rate of the device. Note that we
    // only need to take the state lock to get this value.
    case kAudioDevicePropertyNominalSampleRate: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(Float64));
      *outDataSize = mts::assign<Float64>(outData, get_sample_rate());
    } break;

    // This returns all nominal sample rates the device supports as an array of
    // AudioValueRangeStructs. Note that for discrete sampler rates, the range
    // will have the minimum value equal to the maximum value.

    // Calculate the number of items that have been requested. Note that this
    // number is allowed to be smaller than the actual size of the list. In such
    // case, only that number of items will be returned
    case kAudioDevicePropertyAvailableNominalSampleRates: {
      UInt32 itemCount = inDataSize / sizeof(AudioValueRange);
      itemCount = get_sample_rates((AudioValueRange*)outData, itemCount);
      *outDataSize = itemCount * sizeof(AudioValueRange);
    } break;

    // This returns whether or not the device is visible to clients.
    case kAudioDevicePropertyIsHidden: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, is_hidden());
    } break;

    // This property returns which two channesl to use as left/right for stereo
    // data by default. Note that the channel numbers are 1-based.xz
    case kAudioDevicePropertyPreferredChannelsForStereo: {
      RETURN_SIZE_ERROR_IF(inDataSize < (2 * sizeof(UInt32)));
      ((UInt32*)outData)[0] = 1;
      ((UInt32*)outData)[1] = 2;
      *outDataSize = 2 * sizeof(UInt32);
    } break;

    // This property returns the default AudioChannelLayout to use for the device
    // by default. For this device, we return a stereo ACL.
    case kAudioDevicePropertyPreferredChannelLayout: {
      UInt32 theACLSize = offsetof(AudioChannelLayout, mChannelDescriptions)
          + (get_channel_count() * sizeof(AudioChannelDescription));

      RETURN_SIZE_ERROR_IF(inDataSize < theACLSize);

      AudioChannelLayout* layout = (AudioChannelLayout*)outData;
      layout->mChannelLayoutTag = kAudioChannelLayoutTag_UseChannelDescriptions;
      layout->mChannelBitmap = 0;
      layout->mNumberChannelDescriptions = get_channel_count();

      for (UInt32 i = 0; i < get_channel_count(); i++) {
        AudioChannelDescription& desc = layout->mChannelDescriptions[i];
        desc.mChannelLabel = kAudioChannelLabel_Left + i;
        desc.mChannelFlags = 0;
        desc.mCoordinates[0] = 0;
        desc.mCoordinates[1] = 0;
        desc.mCoordinates[2] = 0;
      }

      *outDataSize = theACLSize;

    } break;

    // This property returns how many frames the HAL should expect to see between
    // successive sample times in the zero time stamps this device provides.
    case kAudioDevicePropertyZeroTimeStampPeriod: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, get_ring_buffer_size());
    } break;

    // This is a CFURL that points to the device's Icon in the plug-in's resource bundle.
    case kAudioDevicePropertyIcon: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(CFURLRef));

      CFBundleRef theBundle = CFBundleGetBundleWithIdentifier(get_bundle_id());

      if (!theBundle) {
        return kAudioHardwareUnspecifiedError;
      }

      CFURLRef theURL = CFBundleCopyResourceURL(theBundle, get_icon_file(), nullptr, nullptr);

      if (!theURL) {
        return kAudioHardwareUnspecifiedError;
      }

      *((CFURLRef*)outData) = theURL;
      *outDataSize = sizeof(CFURLRef);
    } break;

    default:
      return kAudioHardwareUnknownPropertyError;
    }

    return kAudioHardwareNoError;
  }

  inline OSStatus set(const Address* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData,
      UInt32 inDataSize, const void* inData, UInt32* outNumberPropertiesChanged, Address outChangedAddresses[2]) const {
    switch (inAddress->mSelector) {
    case kAudioDevicePropertyNominalSampleRate: {
      RETURN_SIZE_ERROR_IF(inDataSize != sizeof(Float64));
      return set_sample_rate(*(const Float64*)inData);
    } break;

    default:
      return kAudioHardwareUnknownPropertyError;
    }

    return kAudioHardwareNoError;
  }

private:
  AudioObjectID m_plugin;

  inline static constexpr UInt32 get_global_object_list_size() { return ImplObject::objectsDescription.size(); }

  inline static constexpr UInt32 get_input_object_list_size() {
    UInt32 count = 0;
    for (auto o : ImplObject::objectsDescription) {
      count += (o.direction == mts::direction::input);
    }

    return count;
  }

  inline static constexpr UInt32 get_output_object_list_size() {
    UInt32 count = 0;
    for (auto o : ImplObject::objectsDescription) {
      count += (o.direction == mts::direction::output);
    }

    return count;
  }

  inline static constexpr UInt32 get_global_stream_list_size() {
    UInt32 count = 0;
    for (auto o : ImplObject::objectsDescription) {
      count += (o.type == mts::object_type::stream);
    }

    return count;
  }

  inline static constexpr UInt32 get_input_stream_list_size() {
    UInt32 count = 0;
    for (auto o : ImplObject::objectsDescription) {
      count += (o.type == mts::object_type::stream && o.direction == mts::direction::input);
    }

    return count;
  }

  inline static constexpr UInt32 get_output_stream_list_size() {
    UInt32 count = 0;
    for (auto o : ImplObject::objectsDescription) {
      count += (o.type == mts::object_type::stream && o.direction == mts::direction::output);
    }

    return count;
  }

  inline static constexpr UInt32 get_control_list_size() {
    UInt32 count = 0;
    for (auto o : ImplObject::objectsDescription) {
      count += (o.type == mts::object_type::control);
    }

    return count;
  }

  inline static UInt32 get_global_object_list(AudioObjectID* objs, UInt32 itemCount) {
    itemCount = mts::min(itemCount, get_global_object_list_size());
    for (UInt32 i = 0; i < itemCount; i++) {
      objs[i] = ImplObject::objectsDescription[i].id;
    }
    return itemCount;
  }

  inline static UInt32 get_input_object_list(AudioObjectID* objs, UInt32 itemCount) {
    itemCount = mts::min(itemCount, get_input_object_list_size());
    for (UInt32 i = 0, k = 0; k < itemCount; i++) {
      if (ImplObject::objectsDescription[i].direction == mts::direction::input) {
        objs[k] = ImplObject::objectsDescription[i].id;
        k++;
      }
    }
    return itemCount;
  }

  inline static UInt32 get_output_object_list(AudioObjectID* objs, UInt32 itemCount) {
    itemCount = mts::min(itemCount, get_output_object_list_size());
    for (UInt32 i = 0, k = 0; k < itemCount; i++) {
      if (ImplObject::objectsDescription[i].direction == mts::direction::output) {
        objs[k] = ImplObject::objectsDescription[i].id;
        k++;
      }
    }
    return itemCount;
  }

  inline static UInt32 get_global_stream_list(AudioObjectID* objs, UInt32 itemCount) {
    itemCount = mts::min(itemCount, get_global_stream_list_size());
    for (UInt32 i = 0, k = 0; k < itemCount; i++) {
      if (ImplObject::objectsDescription[i].type == mts::object_type::stream) {
        objs[k] = ImplObject::objectsDescription[i].id;
        k++;
      }
    }
    return itemCount;
  }

  inline static UInt32 get_input_stream_list(AudioObjectID* objs, UInt32 itemCount) {
    itemCount = mts::min(itemCount, get_input_stream_list_size());
    for (UInt32 i = 0, k = 0; k < itemCount; i++) {
      if (ImplObject::objectsDescription[i].type == mts::object_type::stream
          && ImplObject::objectsDescription[i].direction == mts::direction::input) {
        objs[k] = ImplObject::objectsDescription[i].id;
        k++;
      }
    }
    return itemCount;
  }

  inline static UInt32 get_output_stream_list(AudioObjectID* objs, UInt32 itemCount) {
    itemCount = mts::min(itemCount, get_output_stream_list_size());
    for (UInt32 i = 0, k = 0; k < itemCount; i++) {
      if (ImplObject::objectsDescription[i].type == mts::object_type::stream
          && ImplObject::objectsDescription[i].direction == mts::direction::output) {
        objs[k] = ImplObject::objectsDescription[i].id;
        k++;
      }
    }
    return itemCount;
  }

  inline static UInt32 get_control_list(AudioObjectID* objs, UInt32 itemCount) {
    itemCount = mts::min(itemCount, get_control_list_size());
    for (UInt32 i = 0, k = 0; k < itemCount; i++) {
      if (ImplObject::objectsDescription[i].type == mts::object_type::control) {
        objs[k] = ImplObject::objectsDescription[i].id;
        k++;
      }
    }

    return itemCount;
  }

  inline const ImplObject* impl() const { return (const ImplObject*)this; }
  inline bool is_hidden() const { return impl()->is_hidden(); }
  inline bool allows_default() const { return impl()->allows_default(); }
  inline Float64 get_sample_rate() const { return impl()->get_sample_rate(); }
  inline UInt32 get_sample_rate_count() const { return impl()->get_sample_rate_count(); }

  inline UInt32 get_sample_rates(AudioValueRange* ranges, UInt32 itemCount) const {
    return impl()->get_sample_rates(ranges, itemCount);
  }

  inline OSStatus set_sample_rate(Float64 sr) const { return impl()->set_sample_rate(sr); }
  inline UInt32 get_channel_count() const { return impl()->get_channel_count(); }
  inline bool is_io_running() const { return impl()->is_io_running(); }
  inline UInt32 get_ring_buffer_size() const { return impl()->get_ring_buffer_size(); }
  inline CFStringRef get_device_name() const { return impl()->get_device_name(); }
  inline CFStringRef get_manufacturer_name() const { return impl()->get_manufacturer_name(); }
  inline CFStringRef get_device_uid() const { return impl()->get_device_uid(); }
  inline CFStringRef get_device_model_uid() const { return impl()->get_device_model_uid(); }
  inline CFStringRef get_bundle_id() const { return impl()->get_bundle_id(); }
  inline CFStringRef get_icon_file() const { return impl()->get_icon_file(); }
};
} // namespace mts::core.
