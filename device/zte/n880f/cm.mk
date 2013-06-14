## Specify phone tech before including full_phone
$(call inherit-product, vendor/cm/config/gsm.mk)

# Release name
PRODUCT_RELEASE_NAME := n880f

# Inherit some common CM stuff.
$(call inherit-product, vendor/cm/config/common_full_phone.mk)

# Inherit device configuration
$(call inherit-product, device/zte/n880f/device_n880f.mk)

## Device identifier. This must come after all inclusions
PRODUCT_DEVICE := n880f
PRODUCT_NAME := cm_n880f
PRODUCT_BRAND := zte
PRODUCT_MODEL := n880f
PRODUCT_MANUFACTURER := zte
