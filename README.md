CyanogenMod 9 device configuration for ZTE Blade.

http://www.modaco.com/topic/355059-cyanogenmod-9/

How to build:
-------------

Initialize CyanogenMod repo:

    repo init -u git://github.com/CyanogenMod/android.git -b ics
    repo sync

Add the following to your cm9/.repo/local_manifest.xml and repo sync again:

    <?xml version="1.0" encoding="UTF-8"?>
    <manifest>
        <project name="KonstaT/android_device_zte_blade" path="device/zte/blade" revision="ics" />
        <project name="KonstaT/proprietary_vendor_zte" path="vendor/zte" revision="ics" />
        <project name="KonstaT/zte-kernel-msm7x27" path="kernel/zte/zte-kernel-msm7x27" revision="ics" />
    </manifest>

Compile:

    . build/envsetup.sh && lunch cm_blade-userdebug
    make bacon -j8

