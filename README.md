CyanogenMod 10 device configuration for ZTE Blade.

http://www.modaco.com/topic/355903-cyanogenmod-10/

How to build:
-------------

Initialize CyanogenMod repo:

    repo init -u git://github.com/CyanogenMod/android.git -b jellybean
    repo sync

Add the following to your cm10/.repo/local_manifest.xml and repo sync again:

    <?xml version="1.0" encoding="UTF-8"?>
    <manifest>
        <project name="KonstaT/android_device_zte_blade" path="device/zte/blade" revision="jellybean" />
        <project name="KonstaT/proprietary_vendor_zte" path="vendor/zte" revision="jellybean" />
        <project name="KonstaT/zte-kernel-msm7x27" path="kernel/zte/zte-kernel-msm7x27" revision="jellybean" />
    </manifest>

Compile:

    . build/envsetup.sh && lunch cm_blade-userdebug
    make bacon -j8

