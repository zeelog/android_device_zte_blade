CyanogenMod 11.0 device configuration for ZTE Blade.


How to build:
-------------

Initialize repo:

    repo init -u git://github.com/androidarmv6/android.git -b cm-11.0
    curl --create-dirs -L -o .repo/local_manifests/local_manifest.xml -O -L https://raw.githubusercontent.com/parmv6/android_local_manifest/cm-11.0/local_manifest.xml
    curl -L -o .repo/local_manifests/local_manifest.xml -O -L https://raw.githubusercontent.com/parmv6/android_local_manifest/cm-11.0/local_manifest.xml
    repo sync

Compile:

    . build/envsetup.sh
    brunch blade

