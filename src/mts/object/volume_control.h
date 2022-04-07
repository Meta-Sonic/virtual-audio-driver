#pragma once
#include "mts/common.h"

namespace mts::core {
/// @class volume_control
///
/// Interface:
/// @code
///     bool set_volume_normalized(Float32 value) const;
///     bool set_volume_decibel(Float32 db) const;
///     Float32 get_volume_decibel() const;
///     Float32 get_volume_normalized() const;
///     Float32 convert_normalized_to_decibel(Float32 value) const;
///     Float32 convert_decibel_to_normalized(Float32 db) const;
///     AudioValueRange get_volume_decibel_range() const;
/// @endcode
///
template <typename ImplObject>
class volume_control : public mts::object {
public:
  inline volume_control(AudioObjectID objID, AudioObjectID deviceID, mts::direction direction)
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
    case kAudioLevelControlPropertyScalarValue:
    case kAudioLevelControlPropertyDecibelValue:
    case kAudioLevelControlPropertyDecibelRange:
    case kAudioLevelControlPropertyConvertScalarToDecibels:
    case kAudioLevelControlPropertyConvertDecibelsToScalar:
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
    case kAudioLevelControlPropertyDecibelRange:
    case kAudioLevelControlPropertyConvertScalarToDecibels:
    case kAudioLevelControlPropertyConvertDecibelsToScalar:
      *outIsSettable = false;
      break;

    case kAudioLevelControlPropertyScalarValue:
    case kAudioLevelControlPropertyDecibelValue:
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

    case kAudioLevelControlPropertyScalarValue:
      *outDataSize = sizeof(Float32);
      break;

    case kAudioLevelControlPropertyDecibelValue:
      *outDataSize = sizeof(Float32);
      break;

    case kAudioLevelControlPropertyDecibelRange:
      *outDataSize = sizeof(AudioValueRange);
      break;

    case kAudioLevelControlPropertyConvertScalarToDecibels:
      *outDataSize = sizeof(Float32);
      break;

    case kAudioLevelControlPropertyConvertDecibelsToScalar:
      *outDataSize = sizeof(Float32);
      break;

    default:
      return kAudioHardwareUnknownPropertyError;
    }

    return kAudioHardwareNoError;
  }

  inline OSStatus get(const Address* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData,
      UInt32 inDataSize, UInt32* outDataSize, void* outData) const {
    switch (inAddress->mSelector) {

    // The base class for kAudioVolumeControlClassID is kAudioLevelControlClassID.
    case kAudioObjectPropertyBaseClass:
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioClassID));
      *outDataSize = mts::assign<AudioClassID>(outData, kAudioLevelControlClassID);
      break;

    // Volume controls are of the class, kAudioVolumeControlClassID.
    case kAudioObjectPropertyClass:
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioClassID));
      *outDataSize = mts::assign<AudioClassID>(outData, kAudioVolumeControlClassID);
      break;

    // The control's owner is the device object.
    case kAudioObjectPropertyOwner:
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioObjectID));
      *outDataSize = mts::assign<AudioObjectID>(outData, m_device);
      break;

    // Controls do not own any objects.
    case kAudioObjectPropertyOwnedObjects:
      *outDataSize = 0;
      break;

    // This property returns the scope that the control is attached to.
    case kAudioControlPropertyScope:
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioObjectPropertyScope));
      *outDataSize = mts::assign<AudioObjectPropertyScope>(outData,
          m_direction == mts::direction::input ? kAudioObjectPropertyScopeInput : kAudioObjectPropertyScopeOutput);
      break;

    // This property returns the element that the control is attached to.
    case kAudioControlPropertyElement:
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioObjectPropertyElement));
      *outDataSize = mts::assign<AudioObjectPropertyElement>(outData, kAudioObjectPropertyElementMain);
      break;

    // This returns the value of the control in the normalized range of 0 to 1.
    // Note that we need to take the state lock to examine the value.
    case kAudioLevelControlPropertyScalarValue:
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(Float32));
      *outDataSize = mts::assign<Float32>(outData, get_volume_normalized());
      break;

    // This returns the dB value of the control.
    // Note that we need to take the state lock to examine the value.
    case kAudioLevelControlPropertyDecibelValue:
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(Float32));
      *outDataSize = mts::assign<Float32>(outData, get_volume_decibel());
      break;

    // This returns the dB range of the control.
    case kAudioLevelControlPropertyDecibelRange:
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioValueRange));
      *outDataSize = mts::assign<AudioValueRange>(outData, get_volume_decibel_range());
      break;

    // This takes the scalar value in outData and converts it to dB.
    case kAudioLevelControlPropertyConvertScalarToDecibels: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(Float32));
      *outDataSize = mts::assign<Float32>(outData, convert_normalized_to_decibel(*(Float32*)outData));

    } break;

    // This takes the dB value in outData and converts it to scalar.
    case kAudioLevelControlPropertyConvertDecibelsToScalar: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(Float32));
      *outDataSize = mts::assign<Float32>(outData, convert_decibel_to_normalized(*(Float32*)outData));

    } break;

    default:
      return kAudioHardwareUnknownPropertyError;
    }

    return kAudioHardwareNoError;
  }

  inline OSStatus set(const Address* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData,
      UInt32 inDataSize, const void* inData, UInt32* outNumberPropertiesChanged, Address outChangedAddresses[2]) const {
    switch (inAddress->mSelector) {
    // For the scalar volume, we clamp the new value to [0, 1]. Note that if this
    // value changes, it implies that the dB value changed too.
    case kAudioLevelControlPropertyScalarValue: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(Float32));

      if (set_volume_normalized(*(const Float32*)inData)) {
        *outNumberPropertiesChanged = 2;
        outChangedAddresses[0].mSelector = kAudioLevelControlPropertyScalarValue;
        outChangedAddresses[0].mScope = kAudioObjectPropertyScopeGlobal;
        outChangedAddresses[0].mElement = kAudioObjectPropertyElementMain;
        outChangedAddresses[1].mSelector = kAudioLevelControlPropertyDecibelValue;
        outChangedAddresses[1].mScope = kAudioObjectPropertyScopeGlobal;
        outChangedAddresses[1].mElement = kAudioObjectPropertyElementMain;
      }
    } break;

    // For the dB value, we first convert it to a scalar value since that is how
    // the value is tracked. Note that if this value changes, it implies that the
    // scalar value changes as well.
    case kAudioLevelControlPropertyDecibelValue: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(Float32));

      if (set_volume_decibel(*(const Float32*)inData)) {
        *outNumberPropertiesChanged = 2;
        outChangedAddresses[0].mSelector = kAudioLevelControlPropertyScalarValue;
        outChangedAddresses[0].mScope = kAudioObjectPropertyScopeGlobal;
        outChangedAddresses[0].mElement = kAudioObjectPropertyElementMain;
        outChangedAddresses[1].mSelector = kAudioLevelControlPropertyDecibelValue;
        outChangedAddresses[1].mScope = kAudioObjectPropertyScopeGlobal;
        outChangedAddresses[1].mElement = kAudioObjectPropertyElementMain;
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
  inline bool set_volume_normalized(Float32 value) const { return impl()->set_volume_normalized(value); }
  inline bool set_volume_decibel(Float32 db) const { return impl()->set_volume_decibel(db); }
  inline Float32 get_volume_decibel() const { return impl()->get_volume_decibel(); }
  inline Float32 get_volume_normalized() const { return impl()->get_volume_normalized(); }

  inline Float32 convert_normalized_to_decibel(Float32 value) const {
    return impl()->convert_normalized_to_decibel(value);
  }

  inline Float32 convert_decibel_to_normalized(Float32 db) const { return impl()->convert_decibel_to_normalized(db); }

  inline AudioValueRange get_volume_decibel_range() const { return impl()->get_volume_decibel_range(); }
};
} // namespace mts::core.
