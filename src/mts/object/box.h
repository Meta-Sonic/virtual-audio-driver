#pragma once
#include "mts/common.h"

namespace mts::core {
/// @class box
///
/// Interface:
/// @code
///     bool is_acquired() const;
///     bool set_acquired(bool acquired) const;
///     bool set_box_name(CFStringRef name) const;
///     CFStringRef get_box_name() const;
///     CFStringRef get_box_model_name() const;
///     CFStringRef get_manufacturer_name() const;
///     CFStringRef get_serial_number() const;
///     CFStringRef get_firmware_version() const;
///     CFStringRef get_box_uid() const;
///     UInt32 get_device_list_count() const;
///     UInt32 get_device_list(AudioObjectID* objs, UInt32 itemCount) const;
/// @endcode
///
template <typename ImplObject>
class box : public mts::object {
public:
  inline box(AudioObjectID objID, AudioObjectID pluginID)
      : object(objID)
      , m_plugin(pluginID) {}

  inline AudioObjectID get_plugin_id() const { return m_plugin; }

  inline bool exists(const Address* inAddress) const {
    switch (inAddress->mSelector) {
    case kAudioObjectPropertyBaseClass:
    case kAudioObjectPropertyClass:
    case kAudioObjectPropertyOwner:
    case kAudioObjectPropertyName:
    case kAudioObjectPropertyModelName:
    case kAudioObjectPropertyManufacturer:
    case kAudioObjectPropertyOwnedObjects:
    case kAudioObjectPropertySerialNumber:
    case kAudioObjectPropertyFirmwareVersion:
    case kAudioBoxPropertyBoxUID:
    case kAudioBoxPropertyTransportType:
    case kAudioBoxPropertyHasAudio:
    case kAudioBoxPropertyHasVideo:
    case kAudioBoxPropertyHasMIDI:
    case kAudioBoxPropertyIsProtected:
    case kAudioBoxPropertyAcquired:
    case kAudioBoxPropertyAcquisitionFailed:
    case kAudioBoxPropertyDeviceList:
      return true;
    }

    return false;
  }

  inline OSStatus is_settable(const Address* inAddress, Boolean* outIsSettable) const {
    switch (inAddress->mSelector) {
    case kAudioObjectPropertyBaseClass:
    case kAudioObjectPropertyClass:
    case kAudioObjectPropertyOwner:
    case kAudioObjectPropertyModelName:
    case kAudioObjectPropertyManufacturer:
    case kAudioObjectPropertyOwnedObjects:
    case kAudioObjectPropertySerialNumber:
    case kAudioObjectPropertyFirmwareVersion:
    case kAudioBoxPropertyBoxUID:
    case kAudioBoxPropertyTransportType:
    case kAudioBoxPropertyHasAudio:
    case kAudioBoxPropertyHasVideo:
    case kAudioBoxPropertyHasMIDI:
    case kAudioBoxPropertyIsProtected:
    case kAudioBoxPropertyAcquisitionFailed:
    case kAudioBoxPropertyDeviceList:
      *outIsSettable = false;
      break;

    case kAudioObjectPropertyName:
    case kAudioBoxPropertyAcquired:
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

    case kAudioObjectPropertyModelName:
      *outDataSize = sizeof(CFStringRef);
      break;

    case kAudioObjectPropertyManufacturer:
      *outDataSize = sizeof(CFStringRef);
      break;

    case kAudioObjectPropertyOwnedObjects:
      *outDataSize = 0;
      break;

    case kAudioObjectPropertySerialNumber:
      *outDataSize = sizeof(CFStringRef);
      break;

    case kAudioObjectPropertyFirmwareVersion:
      *outDataSize = sizeof(CFStringRef);
      break;

    case kAudioBoxPropertyBoxUID:
      *outDataSize = sizeof(CFStringRef);
      break;

    case kAudioBoxPropertyTransportType:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioBoxPropertyHasAudio:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioBoxPropertyHasVideo:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioBoxPropertyHasMIDI:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioBoxPropertyIsProtected:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioBoxPropertyAcquired:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioBoxPropertyAcquisitionFailed:
      *outDataSize = sizeof(UInt32);
      break;

    case kAudioBoxPropertyDeviceList:
      *outDataSize = get_device_list_count() * sizeof(AudioObjectID);
      break;

    default:
      return kAudioHardwareUnknownPropertyError;
    };

    return kAudioHardwareNoError;
  }

  inline OSStatus get(const Address* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData,
      UInt32 inDataSize, UInt32* outDataSize, void* outData) const {
    switch (inAddress->mSelector) {
    // The base class for kAudioBoxClassID is kAudioObjectClassID
    case kAudioObjectPropertyBaseClass: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioClassID));
      *outDataSize = mts::assign<AudioClassID>(outData, kAudioObjectClassID);
    } break;

    // The class is always kAudioBoxClassID for regular drivers
    case kAudioObjectPropertyClass: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioClassID));
      *outDataSize = mts::assign<AudioClassID>(outData, kAudioBoxClassID);
    } break;

    // The owner is the plug-in object
    case kAudioObjectPropertyOwner: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioObjectID));
      *outDataSize = mts::assign<AudioObjectID>(outData, m_plugin);
    } break;

    // This is the human readable name of the maker of the box.
    case kAudioObjectPropertyName: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(CFStringRef));
      *outDataSize = mts::assign<CFStringRef>(outData, get_box_name());
    } break;

    // This is the human readable name of the maker of the box.
    case kAudioObjectPropertyModelName: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(CFStringRef));
      *outDataSize = mts::assign<CFStringRef>(outData, get_box_model_name());
    } break;

    // This is the human readable name of the maker of the box.
    case kAudioObjectPropertyManufacturer: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(CFStringRef));
      *outDataSize = mts::assign<CFStringRef>(outData, get_manufacturer_name());
    } break;

    // This returns the objects directly owned by the object. Boxes don't own anything.
    case kAudioObjectPropertyOwnedObjects: {
      *outDataSize = 0;
    } break;

    // This is the human readable serial number of the box.
    case kAudioObjectPropertySerialNumber: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(CFStringRef));
      *outDataSize = mts::assign<CFStringRef>(outData, get_serial_number());
    } break;

    // This is the human readable firmware version of the box.
    case kAudioObjectPropertyFirmwareVersion: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(CFStringRef));
      *outDataSize = mts::assign<CFStringRef>(outData, get_firmware_version());
    } break;

    // Boxes have UIDs the same as devices
    case kAudioBoxPropertyBoxUID: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(CFStringRef));
      *outDataSize = mts::assign<CFStringRef>(outData, get_box_uid());
    } break;

    // This value represents how the device is attached to the system. This can be
    // any 32 bit integer, but common values for this property are defined in
    // <CoreAudio/AudioHardwareBase.h>
    case kAudioBoxPropertyTransportType: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, kAudioDeviceTransportTypeVirtual);
    } break;

    // Indicates whether or not the box has audio capabilities
    case kAudioBoxPropertyHasAudio: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, 1);
    } break;

    // Indicates whether or not the box has video capabilities
    case kAudioBoxPropertyHasVideo: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, 0);
    } break;

    // Indicates whether or not the box has MIDI capabilities
    case kAudioBoxPropertyHasMIDI:
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, 0);
      break;

    // Indicates whether or not the box has requires authentication to use
    case kAudioBoxPropertyIsProtected: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, 0);
    } break;

    // When set to a non-zero value, the device is acquired for use by the local machine
    case kAudioBoxPropertyAcquired: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, is_acquired());
    } break;

    // This is used for notifications to say when an attempt to acquire a device has failed.
    case kAudioBoxPropertyAcquisitionFailed: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(UInt32));
      *outDataSize = mts::assign<UInt32>(outData, 0);
    } break;

    case kAudioBoxPropertyDeviceList: {
      UInt32 itemCount = inDataSize / sizeof(AudioObjectID);
      itemCount = get_device_list((AudioObjectID*)outData, itemCount);
      *outDataSize = itemCount * sizeof(AudioObjectID);
    } break;

    default:
      return kAudioHardwareUnknownPropertyError;
    }

    return kAudioHardwareNoError;
  }

  inline OSStatus set(const Address* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData,
      UInt32 inDataSize, const void* inData, UInt32* outNumberPropertiesChanged, Address outChangedAddresses[2]) const {
    switch (inAddress->mSelector) {

    // Boxes should allow their name to be editable.
    case kAudioObjectPropertyName: {
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(CFStringRef));

      CFStringRef* name = (CFStringRef*)inData;
      if (set_box_name(*name)) {
        *outNumberPropertiesChanged = 1;
        outChangedAddresses[0].mSelector = kAudioObjectPropertyName;
        outChangedAddresses[0].mScope = kAudioObjectPropertyScopeGlobal;
        outChangedAddresses[0].mElement = kAudioObjectPropertyElementMain;
      }

    } break;

    // When the box is acquired, it means the contents, namely the device, are available to the system.
    case kAudioBoxPropertyAcquired: {
      RETURN_SIZE_ERROR_IF(inDataSize != sizeof(UInt32));

      bool acquired = *((UInt32*)inData) != 0;

      if (set_acquired(acquired)) {
        // This property and the device list property have changed.
        *outNumberPropertiesChanged = 2;
        outChangedAddresses[0].mSelector = kAudioBoxPropertyAcquired;
        outChangedAddresses[0].mScope = kAudioObjectPropertyScopeGlobal;
        outChangedAddresses[0].mElement = kAudioObjectPropertyElementMain;
        outChangedAddresses[1].mSelector = kAudioBoxPropertyDeviceList;
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
  AudioObjectID m_plugin;

  inline const ImplObject* impl() const { return (const ImplObject*)this; }
  inline bool is_acquired() const { return impl()->is_acquired(); }
  inline bool set_acquired(bool acquired) const { return impl()->set_acquired(acquired); }
  inline bool set_box_name(CFStringRef name) const { return impl()->set_box_name(name); }
  inline CFStringRef get_box_name() const { return impl()->get_box_name(); }
  inline CFStringRef get_box_model_name() const { return impl()->get_box_model_name(); }
  inline CFStringRef get_manufacturer_name() const { return impl()->get_manufacturer_name(); }
  inline CFStringRef get_serial_number() const { return impl()->get_serial_number(); }
  inline CFStringRef get_firmware_version() const { return impl()->get_firmware_version(); }
  inline CFStringRef get_box_uid() const { return impl()->get_box_uid(); }
  inline UInt32 get_device_list_count() const { return impl()->get_device_list_count(); }

  inline UInt32 get_device_list(AudioObjectID* objs, UInt32 itemCount) const {
    return impl()->get_device_list(objs, itemCount);
  }
};
} // namespace mts::core.
