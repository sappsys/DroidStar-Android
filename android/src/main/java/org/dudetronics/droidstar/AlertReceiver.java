package org.dudetronics.droidstar;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class AlertReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        if (intent == null) {
            return;
        }
        String kind = intent.getStringExtra("kind");
        if ("fire".equals(kind)) {
            PlatformBridge.showFromAlarm(context, intent);
        } else if ("tap".equals(kind)) {
            PlatformBridge.handleTap(context, intent);
        }
    }
}
