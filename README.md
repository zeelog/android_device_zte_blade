CyanogenMod 10.1 device configuration for ZTE Blade.

How to build:
-------------

Initialize CyanogenMod repo:

    repo init -u git://github.com/CyanogenMod/android.git -b cm-10.1
    repo sync

Add the following to your cm10.1/.repo/local_manifest.xml and repo sync again:

    <?xml version="1.0" encoding="UTF-8"?>
    <manifest>
        <project name="KonstaT/android_device_zte_blade" path="device/zte/blade" revision="cm-10.1" />
        <project name="KonstaT/proprietary_vendor_zte" path="vendor/zte" revision="cm-10.1" />
        <project name="KonstaT/zte-kernel-msm7x27" path="kernel/zte/zte-kernel-msm7x27" revision="cm-10.1" />
    </manifest>

Compile:

    . build/envsetup.sh && lunch cm_blade-userdebug
    make bacon -j8

