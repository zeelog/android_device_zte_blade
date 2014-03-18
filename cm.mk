# Inherit some common CM stuff.
$(call inherit-product, vendor/cm/config/common_mini_phone.mk)

# Inherit device configuration
$(call inherit-product, device/zte/blade/blade.mk)

# Correct bootanimation size for the screen
TARGET_SCREEN_HEIGHT := 320
TARGET_SCREEN_WIDTH := 240
TARGET_BOOTANIMATION_HALF_RES := true

# Device identifier. This must come after all inclusions
PRODUCT_DEVICE := blade
PRODUCT_NAME := cm_blade
PRODUCT_BRAND := ZTE
PRODUCT_MODEL := Blade
PRODUCT_MANUFACTURER := ZTE
PRODUCT_RELEASE_NAME := Blade

# Set build fingerprint / ID / Product Name ect.
PRODUCT_BUILD_PROP_OVERRIDES += \
    BUILD_FINGERPRINT="ZTE/N880E_JB4_2/atlas40:4.2/JOP40C/20121121.110335:user/release-keys" \
    PRIVATE_BUILD_DESC="N880E_JB4_2-user 4.2 JOP40C 20121121.110335 release-keys"
