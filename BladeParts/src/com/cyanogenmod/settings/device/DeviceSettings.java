package com.cyanogenmod.settings.device;

import com.cyanogenmod.settings.device.R;

import android.os.Bundle;
import android.preference.PreferenceActivity;

public class DeviceSettings extends PreferenceActivity {

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        addPreferencesFromResource(R.xml.bladeparts);

    }

}
