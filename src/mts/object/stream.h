#pragma once
#include "mts/common.h"

namespace mts::core {
/// @class stream
///
/// Interface:
/// @code
///     UInt32 get_sample_rate_count() const;
///     bool is_active() const;
///     bool set_active(bool active) const;
///     void get_basic_description(AudioStreamBasicDescription& desc) const;
///     void get_ranged_descriptions(AudioStreamRangedDescription* desc, UInt32 itemCount) const;
///     OSStatus set_format(const AudioStreamBasicDescription* desc) const;
/// @endcode
///
template <typename ImplObject>
class stream : public mts::object {
public:
  inline stream(AudioObjectID objID, AudioObjectID deviceID, mts::direction direction)
      : object(objID)
      , m_device(deviceID)
      , m_direction(direction) {}

  inline AudioObjectID get_device_id() const { return m_device; }

  inline mts::direction get_direction() const { return m_direction; }

  inline bool exists(const Address* inAddress) const {
    switch (inAddress->mSelector) {
    case kAudioObjectPropertyBaseClass:
    case kAudioObjectPropertyClass:
    case kAudioObjectPropertyOwner:
    case kAudioObjectPropertyOwnedObjects:
    case kAudioStreamPropertyIsActive:
    case kAudioStreamPropertyDirection:
    case kAudioStreamPropertyTerminalType:
    case kAudioStreamPropertyStartingChannel:
    case kAudioStreamPropertyLatency:
    case kAudioStreamPropertyVirtualFormat:
    case kAudioStreamPropertyPhysicalFormat:
    case kAudioStreamPropertyAvailableVirtualFormats:
    case kAudioStreamPropertyAvailablePhysicalFormats:
      return true;
    }

    return false;
  }

  inline OSStatus is_settable(const Address* inAddress, Boolean* outIsSettable) const {
    switch (inAddress->mSelector) {
    case kAudioObjectPropertyBaseClass:
    case kAudioObjectPropertyClass:
    case kAudioObjectPropertyOwner:
    case kAudioObjectPropertyOwnedObjects:
    case kAudioStreamPropertyDirection:
    case kAudioStreamPropertyTerminalType:
    case kAudioStreamPropertyStartingChannel:
    case kAudioStreamPropertyLatency:
    case kAudioStreamPropertyAvailableVirtualFormats:
    case kAudioStreamPropertyAvailablePhysicalFormats:
      *outIsSettable = false;
      break;

    case kAudioStreamPropertyIsActive:
    case kAudioStreamPropertyVirtualFormat:
    case kAudioStreamPropertyPhysicalFormat:
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

    case kAudioObjectPropertyOwnedObjects:
      *outDataSize = 0;
      break;

    case kAudioStreamPropertyIsActive:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioStreamPropertyDirection:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioStreamPropertyTerminalType:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioStreamPropertyStartingChannel:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioStreamPropertyLatency:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioStreamPropertyVirtualFormat:
    case kAudioStreamPropertyPhysicalFormat:
      *outDataSize = sizeof(AudioStreamBasicDescription);
      break;

    case kAudioStreamPropertyAvailableVirtualFormats:
    case kAudioStreamPropertyAvailablePhysicalFormats:
      *outDataSize = get_sample_rate_count() * sizeof(AudioStreamRangedDescription);
      break;

    default:
      return kAudioHardwareUnknownPropertyError;
    }

    return kAudioHardwareNoError;
  }

  inline OSStatus get(const Address* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData,
      UInt32 inDataSize, UInt32* outDataSize, void* outData) const {
    switch (inAddress->mSelector) {
    // The base class for kAudioStreamClassID is kAudioObjectClassID.
    case kAudioObjectPropertyBaseClass: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioClassID));
      *outDataSize = mts::assign<AudioClassID>(outData, kAudioObjectClassID);
    } break;

    // The class is always kAudioStreamClassID for streams created by drivers.
    case kAudioObjectPropertyClass: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioClassID));
      *outDataSize = mts::assign<AudioClassID>(outData, kAudioStreamClassID);
    } break;

    // The stream's owner is the device object.
    case kAudioObjectPropertyOwner: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioObjectID));
      *outDataSize = mts::assign<AudioObjectID>(outData, m_device);
    } break;

    case kAudioObjectPropertyOwnedObjects: {
      // Streams do not own any objects.
      *outDataSize = 0;
    } break;

    // This property tells the device whether or not the given stream is going to
    // be used for IO. Note that we need to take the state lock to examine this
    // value.
    case kAudioStreamPropertyIsActive: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, is_active());
    } break;

    // This returns whether the stream is an input stream or an output stream.
    case kAudioStreamPropertyDirection: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, m_direction == mts::direction::input);
    } break;

    // This returns a value that indicates what is at the other end of the stream
    // such as a speaker or headphones, or a microphone. Values for this property
    // are defined in <CoreAudio/AudioHardwareBase.h>
    case kAudioStreamPropertyTerminalType: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData,
          m_direction == mts::direction::input ? kAudioStreamTerminalTypeMicrophone : kAudioStreamTerminalTypeSpeaker);
    } break;

    // This property returns the absolute channel number for the first channel in
    // the stream. For exmaple, if a device has two output streams with two
    // channels each, then the starting channel number for the first stream is 1
    // and ths starting channel number fo the second stream is 3.
    case kAudioStreamPropertyStartingChannel: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, 1);
    } break;

    // This property returns any additonal presentation latency the stream has.
    case kAudioStreamPropertyLatency: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, 0);
    } break;

    // This returns the current format of the stream in an
    // AudioStreamBasicDescription. Note that we need to hold the state lock to get
    // this value.
    // Note that for devices that don't override the mix operation, the virtual
    // format has to be the same as the physical format.
    case kAudioStreamPropertyVirtualFormat:
    case kAudioStreamPropertyPhysicalFormat: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioStreamBasicDescription));
      get_basic_description(*(AudioStreamBasicDescription*)outData);
      *outDataSize = sizeof(AudioStreamBasicDescription);
    } break;

    // This returns an array of AudioStreamRangedDescriptions that describe what
    // formats are supported.
    case kAudioStreamPropertyAvailableVirtualFormats:
    case kAudioStreamPropertyAvailablePhysicalFormats: {
      // Calculate the number of items that have been requested. Note that this
      // number is allowed to be smaller than the actual size of the list. In such
      // case, only that number of items will be returned.
      UInt32 itemCount = mts::min<UInt32>(inDataSize / sizeof(AudioStreamRangedDescription), get_sample_rate_count());
      AudioStreamRangedDescription* desc = (AudioStreamRangedDescription*)outData;
      get_ranged_descriptions(desc, itemCount);
      *outDataSize = itemCount * sizeof(AudioStreamRangedDescription);
    } break;

    default:
      return kAudioHardwareUnknownPropertyError;
    }

    return kAudioHardwareNoError;
  }

  inline OSStatus set(const Address* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData,
      UInt32 inDataSize, const void* inData, UInt32* outNumberPropertiesChanged, Address outChangedAddresses[2]) const {
    switch (inAddress->mSelector) {
    // Changing the active state of a stream doesn't affect IO or change the structure
    // so we can just save the state and send the notification.
    case kAudioStreamPropertyIsActive: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));

      bool active = *(const UInt32*)inData;

      if (set_active(active)) {
        *outNumberPropertiesChanged = 1;
        outChangedAddresses[0].mSelector = kAudioStreamPropertyIsActive;
        outChangedAddresses[0].mScope = kAudioObjectPropertyScopeGlobal;
        outChangedAddresses[0].mElement = kAudioObjectPropertyElementMain;
      }
    } break;

    // Changing the stream format needs to be handled via the
    // RequestConfigChange/PerformConfigChange machinery. Note that because this
    // device only supports 2 channel 32 bit float data, the only thing that can
    // change is the sample rate.
    case kAudioStreamPropertyVirtualFormat:
    case kAudioStreamPropertyPhysicalFormat: {
      RETURN_SIZE_ERROR_IF(inDataSize != sizeof(AudioStreamBasicDescription));
      const AudioStreamBasicDescription* desc = (const AudioStreamBasicDescription*)inData;
      return set_format(desc);
    } break;

    default:
      return kAudioHardwareUnknownPropertyError;
    }

    return kAudioHardwareNoError;
  }

private:
  AudioObjectID m_device;
  mts::direction m_direction;

  inline const ImplObject* impl() const { return (const ImplObject*)this; }
  inline UInt32 get_sample_rate_count() const { return impl()->get_sample_rate_count(); }
  inline bool is_active() const { return impl()->is_active(); }
  inline bool set_active(bool active) const { return impl()->set_active(active); }
  inline void get_basic_description(AudioStreamBasicDescription& desc) const { impl()->get_basic_description(desc); }

  inline void get_ranged_descriptions(AudioStreamRangedDescription* desc, UInt32 itemCount) const {
    impl()->get_ranged_descriptions(desc, itemCount);
  }

  inline OSStatus set_format(const AudioStreamBasicDescription* desc) const { return impl()->set_format(desc); }
};
} // namespace mts::core.
