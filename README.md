CyanogenMod 10.1 device configuration for ZTE Blade.

http://www.modaco.com/topic/359832-cyanogenmod-10.1

How to build:
-------------

Initialize repo:

    repo init -u git://github.com/CyanogenMod/android.git -b cm-10.1
    curl --create-dirs -L -o .repo/local_manifests/local_manifest.xml -O -L https://raw.github.com/legaCyMod/android_local_manifest/cm-10.1/local_manifest.xml
    curl -L -o .repo/local_manifests/manifest_zte_blade.xml -O -L https://raw.github.com/legaCyMod/android_local_manifest/cm-10.1/manifest_zte_blade.xml
    repo sync

Compile:

    . build/envsetup.sh
    brunch blade

