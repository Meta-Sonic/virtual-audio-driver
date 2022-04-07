#pragma once
#include "mts/common.h"

namespace mts::core {
/// @class mute_control
///
/// Interface:
/// @code
///     bool is_muted() const;
///     void set_muted(bool muted) const;
/// @endcode
///
template <typename ImplObject>
class mute_control : public mts::object {
public:
  inline mute_control(AudioObjectID objID, AudioObjectID deviceID, mts::direction direction)
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
    case kAudioControlPropertyScope:
    case kAudioControlPropertyElement:
    case kAudioBooleanControlPropertyValue:
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
    case kAudioControlPropertyScope:
    case kAudioControlPropertyElement:
      *outIsSettable = false;
      break;

    case kAudioBooleanControlPropertyValue:
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

    case kAudioControlPropertyScope:
      *outDataSize = sizeof(AudioObjectPropertyScope);
      break;

    case kAudioControlPropertyElement:
      *outDataSize = sizeof(AudioObjectPropertyElement);
      break;

    case kAudioBooleanControlPropertyValue:
      *outDataSize = sizeof(UInt32);
      break;

    default:
      return kAudioHardwareUnknownPropertyError;
    }

    return kAudioHardwareNoError;
  }

  inline OSStatus get(const Address* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData,
      UInt32 inDataSize, UInt32* outDataSize, void* outData) const {
    switch (inAddress->mSelector) {

    // The base class for kAudioMuteControlClassID is kAudioBooleanControlClassID.
    case kAudioObjectPropertyBaseClass: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioClassID));
      *outDataSize = mts::assign<AudioClassID>(outData, kAudioBooleanControlClassID);
    } break;

    // Mute controls are of the class, kAudioMuteControlClassID
    case kAudioObjectPropertyClass: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioClassID));
      *outDataSize = mts::assign<AudioClassID>(outData, kAudioMuteControlClassID);
    } break;

    // The control's owner is the device object
    case kAudioObjectPropertyOwner: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioObjectID));
      *outDataSize = mts::assign<AudioObjectID>(outData, m_device);
    } break;

    // Controls do not own any objects.
    case kAudioObjectPropertyOwnedObjects: {
      *outDataSize = 0;
    } break;

    // This property returns the scope that the control is attached to.
    case kAudioControlPropertyScope: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioObjectPropertyScope));
      *outDataSize = mts::assign<AudioObjectPropertyScope>(outData,
          m_direction == mts::direction::input ? kAudioObjectPropertyScopeInput : kAudioObjectPropertyScopeOutput);
    } break;

    // This property returns the element that the control is attached to.
    case kAudioControlPropertyElement: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioObjectPropertyElement));
      *outDataSize = mts::assign<AudioObjectPropertyElement>(outData, kAudioObjectPropertyElementMain);
    } break;

    // This returns the value of the mute control where 0 means that mute is off
    // and audio can be heard and 1 means that mute is on and audio cannot be heard.
    // Note that we need to take the state lock to examine this value.
    case kAudioBooleanControlPropertyValue: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, is_muted());
    } break;

    default:
      return kAudioHardwareUnknownPropertyError;
    }
    return kAudioHardwareNoError;
  }

  inline OSStatus set(const Address* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData,
      UInt32 inDataSize, const void* inData, UInt32* outNumberPropertiesChanged, Address outChangedAddresses[2]) const {
    switch (inAddress->mSelector) {
    case kAudioBooleanControlPropertyValue: {
      RETURN_SIZE_ERROR_IF(inDataSize != sizeof(UInt32));

      bool muted = *((const UInt32*)inData) != 0;
      if (is_muted() != muted) {
        set_muted(muted);
        *outNumberPropertiesChanged = 1;
        outChangedAddresses[0].mSelector = kAudioBooleanControlPropertyValue;
        outChangedAddresses[0].mScope = kAudioObjectPropertyScopeGlobal;
        outChangedAddresses[0].mElement = kAudioObjectPropertyElementMain;
      }
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
  inline bool is_muted() const { return impl()->is_muted(); }
  inline void set_muted(bool muted) const { impl()->set_muted(muted); }
};
} // namespace mts::core.
