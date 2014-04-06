package org.omnirom.device;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class BladePartsStartup extends BroadcastReceiver
{
    @Override
    public void onReceive(final Context context, final Intent bootintent) {
        DeviceSettings.onStartup(context);
    }
}
