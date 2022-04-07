#pragma once
#include "mts/common.h"

namespace mts::core {
/// @class plugin
///
/// Interface:
/// @code
///     CFStringRef get_resource_bundle() const;
///     AudioObjectID get_device_from_uid(CFStringRef uid) const;
///     AudioObjectID get_box_from_uid(CFStringRef uid) const;
///     UInt32 get_device_list_size() const;
///     UInt32 get_box_list_size() const;
///     UInt32 get_object_list_size() const;
///     UInt32 get_device_list(AudioObjectID* objs, UInt32 itemCount) const;
///     UInt32 get_box_list(AudioObjectID* objs, UInt32 itemCount) const;
///     UInt32 get_object_list(AudioObjectID* objs, UInt32 itemCount) const;
///     CFStringRef get_manufacturer_name() const;
/// @endcode
///
template <typename ImplObject>
class plugin : public mts::object {
public:
  inline plugin(AudioObjectID objID)
      : object(objID) {}

  inline bool exists(const Address* inAddress) const {
    switch (inAddress->mSelector) {
    case kAudioObjectPropertyBaseClass:
    case kAudioObjectPropertyClass:
    case kAudioObjectPropertyOwner:
    case kAudioObjectPropertyManufacturer:
    case kAudioObjectPropertyOwnedObjects:
    case kAudioPlugInPropertyBoxList:
    case kAudioPlugInPropertyTranslateUIDToBox:
    case kAudioPlugInPropertyDeviceList:
    case kAudioPlugInPropertyTranslateUIDToDevice:
    case kAudioPlugInPropertyResourceBundle:
      return true;
    }

    return false;
  }

  inline OSStatus is_settable(const Address* inAddress, Boolean* outIsSettable) const {
    switch (inAddress->mSelector) {
    case kAudioObjectPropertyBaseClass:
    case kAudioObjectPropertyClass:
    case kAudioObjectPropertyOwner:
    case kAudioObjectPropertyManufacturer:
    case kAudioObjectPropertyOwnedObjects:
    case kAudioPlugInPropertyBoxList:
    case kAudioPlugInPropertyTranslateUIDToBox:
    case kAudioPlugInPropertyDeviceList:
    case kAudioPlugInPropertyTranslateUIDToDevice:
    case kAudioPlugInPropertyResourceBundle:
      *outIsSettable = false;
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

    case kAudioObjectPropertyManufacturer:
      *outDataSize = sizeof(CFStringRef);
      break;

    case kAudioObjectPropertyOwnedObjects:
      *outDataSize = get_object_list_size() * sizeof(AudioClassID);
      break;

    case kAudioPlugInPropertyBoxList:
      *outDataSize = get_box_list_size() * sizeof(AudioClassID);
      break;

    case kAudioPlugInPropertyTranslateUIDToBox:
      *outDataSize = sizeof(AudioObjectID);
      break;

    case kAudioPlugInPropertyDeviceList:
      *outDataSize = get_device_list_size() * sizeof(AudioClassID);
      break;

    case kAudioPlugInPropertyTranslateUIDToDevice:
      *outDataSize = sizeof(AudioObjectID);
      break;

    case kAudioPlugInPropertyResourceBundle:
      *outDataSize = sizeof(CFStringRef);
      break;

    default:
      return kAudioHardwareUnknownPropertyError;
    }

    return kAudioHardwareNoError;
  }

  inline OSStatus get(const Address* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData,
      UInt32 inDataSize, UInt32* outDataSize, void* outData) const {
    switch (inAddress->mSelector) {

    // The base class for kAudioPlugInClassID is kAudioObjectClassID.
    case kAudioObjectPropertyBaseClass:
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioClassID));
      *outDataSize = mts::assign<AudioClassID>(outData, kAudioObjectClassID);
      break;

    // The class is always kAudioPlugInClassID for regular drivers.
    case kAudioObjectPropertyClass:
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioClassID));
      *outDataSize = mts::assign<AudioClassID>(outData, kAudioPlugInClassID);
      break;

    // The plug-in doesn't have an owning object.
    case kAudioObjectPropertyOwner:
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioObjectID));
      *outDataSize = mts::assign<AudioObjectID>(outData, kAudioObjectUnknown);
      break;

    // This is the human readable name of the maker of the plug-in.
    case kAudioObjectPropertyManufacturer:
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(CFStringRef));
      *outDataSize = mts::assign<CFStringRef>(outData, get_manufacturer_name());
      break;

    // Calculate the number of items that have been requested. Note that this
    // number is allowed to be smaller than the actual size of the list. In such
    // case, only that number of items will be returned
    case kAudioObjectPropertyOwnedObjects: {
      UInt32 itemCount = inDataSize / sizeof(AudioObjectID);
      itemCount = get_object_list((AudioObjectID*)outData, itemCount);
      *outDataSize = itemCount * sizeof(AudioClassID);
    } break;

    // Calculate the number of items that have been requested. Note that this
    // number is allowed to be smaller than the actual size of the list. In such
    // case, only that number of items will be returned
    case kAudioPlugInPropertyBoxList: {
      UInt32 itemCount = inDataSize / sizeof(AudioObjectID);
      itemCount = get_box_list((AudioObjectID*)outData, itemCount);
      *outDataSize = itemCount * sizeof(AudioClassID);
    } break;

    // This property takes the CFString passed in the qualifier and converts that
    // to the object ID of the box it corresponds to. For this driver, there is
    // just the one box. Note that it is not an error if the string in the
    // qualifier doesn't match any devices. In such case, kAudioObjectUnknown is
    // the object ID to return.
    case kAudioPlugInPropertyTranslateUIDToBox:
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioObjectID));
      RETURN_SIZE_ERROR_IF(inQualifierDataSize != sizeof(CFStringRef));
      RETURN_SIZE_ERROR_IF(!inQualifierData);
      *((AudioObjectID*)outData) = get_box_from_uid(*(CFStringRef*)inQualifierData);
      *outDataSize = sizeof(AudioObjectID);
      break;

    // Calculate the number of items that have been requested. Note that this
    // number is allowed to be smaller than the actual size of the list. In such
    // case, only that number of items will be returned
    case kAudioPlugInPropertyDeviceList: {
      UInt32 itemCount = inDataSize / sizeof(AudioObjectID);
      itemCount = get_device_list((AudioObjectID*)outData, itemCount);
      *outDataSize = itemCount * sizeof(AudioClassID);
    } break;

    // This property takes the CFString passed in the qualifier and converts that
    // to the object ID of the device it corresponds to. For this driver, there is
    // just the one device. Note that it is not an error if the string in the
    // qualifier doesn't match any devices. In such case, kAudioObjectUnknown is
    // the object ID to return.
    case kAudioPlugInPropertyTranslateUIDToDevice:
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioObjectID));
      RETURN_SIZE_ERROR_IF(inQualifierDataSize != sizeof(CFStringRef));
      RETURN_SIZE_ERROR_IF(!inQualifierData);
      *((AudioObjectID*)outData) = get_device_from_uid(*(CFStringRef*)inQualifierData);
      *outDataSize = sizeof(AudioObjectID);
      break;

    // The resource bundle is a path relative to the path of the plug-in's bundle.
    // To specify that the plug-in bundle itself should be used, we just return the
    // empty string.
    case kAudioPlugInPropertyResourceBundle:
      RETURN_SIZE_ERROR_IF(inDataSize < sizeof(AudioObjectID));
      *outDataSize = mts::assign<CFStringRef>(outData, get_resource_bundle());
      break;

    default:
      return kAudioHardwareUnknownPropertyError;
    };

    return kAudioHardwareNoError;
  }

  inline OSStatus set(const Address* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData,
      UInt32 inDataSize, const void* inData, UInt32* outNumberPropertiesChanged, Address outChangedAddresses[2]) const {
    return kAudioHardwareUnknownPropertyError;
  }

private:
  inline const ImplObject* impl() const { return (const ImplObject*)this; }
  inline CFStringRef get_resource_bundle() const { return impl()->get_resource_bundle(); }
  inline AudioObjectID get_device_from_uid(CFStringRef uid) const { return impl()->get_device_from_uid(uid); }
  inline AudioObjectID get_box_from_uid(CFStringRef uid) const { return impl()->get_box_from_uid(uid); }
  inline UInt32 get_device_list_size() const { return impl()->get_device_list_size(); }
  inline UInt32 get_box_list_size() const { return impl()->get_box_list_size(); }
  inline UInt32 get_object_list_size() const { return impl()->get_object_list_size(); }

  inline UInt32 get_device_list(AudioObjectID* objs, UInt32 itemCount) const {
    return impl()->get_device_list(objs, itemCount);
  }

  inline UInt32 get_box_list(AudioObjectID* objs, UInt32 itemCount) const {
    return impl()->get_box_list(objs, itemCount);
  }

  inline UInt32 get_object_list(AudioObjectID* objs, UInt32 itemCount) const {
    return impl()->get_object_list(objs, itemCount);
  }

  inline CFStringRef get_manufacturer_name() const { return impl()->get_manufacturer_name(); }
};
} // namespace mts::core.
