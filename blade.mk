$(call inherit-product, $(SRC_TARGET_DIR)/product/languages_full.mk)

# The gps config appropriate for this device
$(call inherit-product, device/common/gps/gps_us_supl.mk)

$(call inherit-product-if-exists, vendor/zte/blade/blade-vendor.mk)

DEVICE_PACKAGE_OVERLAYS := device/zte/blade/overlay

PRODUCT_AAPT_CONFIG := normal hdpi
PRODUCT_AAPT_PREF_CONFIG := hdpi

# Graphics
PRODUCT_PACKAGES += \
    gralloc.msm7x27 \
    hwcomposer.msm7x27 \
    copybit.msm7x27

# OMX
PRODUCT_PACKAGES += \
    libmm-omxcore \
    libOmxCore \
    libstagefrighthw

# Camera
PRODUCT_PACKAGES += \
    camera.msm7x27

# GPS
PRODUCT_PACKAGES += \
    librpc \
    gps.blade

# Blade specific
PRODUCT_PACKAGES += \
    lights.blade \
    sensors.blade \
    BladeParts

# Power HAL
PRODUCT_PACKAGES += \
    power.msm7x27

# Audio
PRODUCT_PACKAGES += \
    audio.primary.blade \
    audio_policy.blade \
    audio.a2dp.default \
    libaudioutils

# Live Wallpapers
PRODUCT_PACKAGES += \
    LiveWallpapersPicker \
    librs_jni

# FM Radio
PRODUCT_PACKAGES += \
    libfmradio.si4708 \
    Effem

# Other
PRODUCT_PACKAGES += \
    make_ext4fs \
    setup_fs \
    dexpreopt

# Enable repeatable keys in CWM
PRODUCT_PROPERTY_OVERRIDES += \
    ro.cwm.enable_key_repeat=true

# Ramdisk
PRODUCT_COPY_FILES += \
    device/zte/blade/ramdisk/init.blade.rc:root/init.blade.rc \
    device/zte/blade/ramdisk/init.blade.usb.rc:root/init.blade.usb.rc \
    device/zte/blade/ramdisk/ueventd.blade.rc:root/ueventd.blade.rc

# Prebuilt
PRODUCT_COPY_FILES += \
    $(call find-copy-subdir-files,*,device/zte/blade/prebuilt/system,system)

# These are the hardware-specific features
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.distinct.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.distinct.xml \
    frameworks/native/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
    frameworks/native/data/etc/com.stericsson.hardware.fm.receiver.xml:system/etc/permissions/com.stericsson.hardware.fm.receiver.xml

$(call inherit-product, frameworks/native/build/phone-hdpi-512-dalvik-heap.mk)
$(call inherit-product, build/target/product/full.mk)

PRODUCT_BUILD_PROP_OVERRIDES += BUILD_UTC_DATE=0
PRODUCT_NAME := full_blade
PRODUCT_DEVICE := blade
PRODUCT_MANUFACTURER := ZTE
PRODUCT_MODEL := Blade
