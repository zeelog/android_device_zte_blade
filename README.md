CyanogenMod 10.1 device configuration for ZTE Blade.

http://www.modaco.com/topic/359832-cyanogenmod-10.1

How to build:
-------------

Initialize repo:

    repo init -u git://github.com/CyanogenMod/android.git -b cm-10.1
    curl -L -o .repo/local_manifests/local_manifest.xml -O -L https://raw.github.com/legaCyMod/android_local_manifest/cm-10.1/local_manifest.xml
    repo sync

Compile:

    . build/envsetup.sh && lunch cm_blade-userdebug
    make bacon -j8

