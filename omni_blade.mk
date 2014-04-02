# Inherit from our custom product configuration
$(call inherit-product, vendor/omni/config/common.mk)
$(call inherit-product, vendor/omni/config/gsm.mk)

PRODUCT_AAPT_PREF_CONFIG := hdpi

# Inherit device configuration
$(call inherit-product, device/zte/blade/blade.mk)

# Device identifier. This must come after all inclusions
PRODUCT_DEVICE := blade
PRODUCT_NAME := omni_blade
PRODUCT_BRAND := ZTE
PRODUCT_MODEL := Blade
PRODUCT_MANUFACTURER := ZTE
PRODUCT_RELEASE_NAME := Blade
