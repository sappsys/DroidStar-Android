package org.dudetronics.droidstar;

import android.app.AlarmManager;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Build;

import java.util.HashSet;
import java.util.Set;

public final class PlatformBridge {
    public static final String CHANNEL_ALERTS = "droidstar_alerts";
    public static final String CHANNEL_LISTEN = "droidstar_listen";
    private static final String PREFS = "droidstar_platform";
    private static final int SMS_NOTIFICATION_ID = 4100;

    private static boolean sNativeReady = false;

    private PlatformBridge() {}

    public static native void nativeNetTapped(String netId);
    public static native void nativeSmsTapped(String threadId);
    public static native void nativeNetDelivered(String netId);

    public static void setNativeReady(Context context, boolean ready) {
        sNativeReady = ready;
        if (ready) {
            ensureChannels(context);
            drainPendingTaps(context);
        }
    }

    public static void ensureChannels(Context context) {
        if (Build.VERSION.SDK_INT < 26) {
            return;
        }
        NotificationManager nm = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        NotificationChannel alerts = new NotificationChannel(CHANNEL_ALERTS, "DroidStar alerts", NotificationManager.IMPORTANCE_HIGH);
        alerts.setDescription("Net reminders and DMR messages");
        NotificationChannel listen = new NotificationChannel(CHANNEL_LISTEN, "DroidStar listening", NotificationManager.IMPORTANCE_LOW);
        listen.setDescription("Keeps receive audio running");
        nm.createNotificationChannel(alerts);
        nm.createNotificationChannel(listen);
    }

    public static void postSms(Context context, String threadId, String title, String body) {
        ensureChannels(context);
        Notification.Builder builder = notification(context, CHANNEL_ALERTS, title, body);
        builder.setContentIntent(tapIntent(context, "sms", threadId == null ? "" : threadId, 8100));
        builder.setAutoCancel(true);
        notify(context, SMS_NOTIFICATION_ID, builder.build());
    }

    public static void showFromAlarm(Context context, Intent intent) {
        ensureChannels(context);
        String id = intent.getStringExtra("id");
        String netId = intent.getStringExtra("netId");
        String title = intent.getStringExtra("title");
        String body = intent.getStringExtra("body");
        Notification.Builder builder = notification(context, CHANNEL_ALERTS, title, body);
        builder.setContentIntent(tapIntent(context, "net", netId == null ? "" : netId, 8200 + stableCode(id)));
        builder.setAutoCancel(true);
        notify(context, 5000 + stableCode(id), builder.build());
        if (sNativeReady && netId != null) {
            nativeNetDelivered(netId);
        }
    }

    public static void handleTap(Context context, Intent intent) {
        String kind = intent.getStringExtra("tapKind");
        String id = intent.getStringExtra("tapId");
        if (kind == null || id == null) {
            return;
        }
        if (!dispatchTap(kind, id)) {
            SharedPreferences prefs = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
            prefs.edit().putString("pending_kind", kind).putString("pending_id", id).apply();
        }
        Intent launch = context.getPackageManager().getLaunchIntentForPackage(context.getPackageName());
        if (launch != null) {
            launch.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_CLEAR_TOP);
            try {
                context.startActivity(launch);
            } catch (RuntimeException ignored) {
            }
        }
    }

    public static void drainPendingTaps(Context context) {
        SharedPreferences prefs = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
        String kind = prefs.getString("pending_kind", null);
        String id = prefs.getString("pending_id", null);
        if (kind == null || id == null) {
            return;
        }
        if (dispatchTap(kind, id)) {
            prefs.edit().remove("pending_kind").remove("pending_id").apply();
        }
    }

    public static void replaceAlerts(Context context, String[] ids, String[] netIds, String[] titles, String[] bodies, long[] whenMs) {
        AlarmManager am = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        SharedPreferences prefs = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
        Set<String> previous = prefs.getStringSet("alert_ids", new HashSet<String>());
        for (String oldId : previous) {
            PendingIntent pi = alarmIntent(context, oldId, "", "", "", 0L, PendingIntent.FLAG_NO_CREATE);
            if (pi != null) {
                am.cancel(pi);
                pi.cancel();
            }
        }
        Set<String> next = new HashSet<String>();
        int count = ids == null ? 0 : ids.length;
        long now = System.currentTimeMillis();
        for (int i = 0; i < count; ++i) {
            if (whenMs[i] <= now + 1500L) {
                continue;
            }
            PendingIntent pi = alarmIntent(context, ids[i], netIds[i], titles[i], bodies[i], whenMs[i], PendingIntent.FLAG_UPDATE_CURRENT);
            try {
                if (Build.VERSION.SDK_INT >= 31 && !am.canScheduleExactAlarms()) {
                    am.setAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, whenMs[i], pi);
                } else {
                    am.setExactAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, whenMs[i], pi);
                }
            } catch (SecurityException ex) {
                am.setAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, whenMs[i], pi);
            }
            next.add(ids[i]);
        }
        prefs.edit().putStringSet("alert_ids", next).apply();
    }

    private static boolean dispatchTap(String kind, String id) {
        if (!sNativeReady) {
            return false;
        }
        if ("sms".equals(kind)) {
            nativeSmsTapped(id);
            return true;
        }
        if ("net".equals(kind)) {
            nativeNetTapped(id);
            return true;
        }
        return true;
    }

    private static PendingIntent alarmIntent(Context context, String id, String netId, String title, String body, long whenMs, int mutabilityBase) {
        Intent intent = new Intent(context, AlertReceiver.class);
        intent.setAction("org.dudetronics.droidstar.ALERT_FIRE");
        intent.setData(Uri.parse("droidstar://alert/" + Uri.encode(id)));
        intent.putExtra("kind", "fire");
        intent.putExtra("id", id);
        intent.putExtra("netId", netId);
        intent.putExtra("title", title);
        intent.putExtra("body", body);
        intent.putExtra("when", whenMs);
        int flags = mutabilityBase | PendingIntent.FLAG_IMMUTABLE;
        return PendingIntent.getBroadcast(context, stableCode(id), intent, flags);
    }

    private static PendingIntent tapIntent(Context context, String kind, String id, int requestCode) {
        Intent intent = new Intent(context, AlertReceiver.class);
        intent.setAction("org.dudetronics.droidstar.ALERT_TAP");
        intent.setData(Uri.parse("droidstar://tap/" + kind + "/" + Uri.encode(id)));
        intent.putExtra("kind", "tap");
        intent.putExtra("tapKind", kind);
        intent.putExtra("tapId", id);
        return PendingIntent.getBroadcast(context, requestCode, intent, PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
    }

    private static Notification.Builder notification(Context context, String channel, String title, String body) {
        Notification.Builder builder = new Notification.Builder(context, channel);
        int icon = context.getApplicationInfo().icon;
        if (icon == 0) {
            icon = android.R.drawable.stat_sys_headset;
        }
        return builder.setSmallIcon(icon)
                .setContentTitle(title == null ? "DroidStar" : title)
                .setContentText(body == null ? "" : body)
                .setStyle(new Notification.BigTextStyle().bigText(body == null ? "" : body));
    }

    private static void notify(Context context, int id, Notification notification) {
        NotificationManager nm = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        nm.notify(id, notification);
    }

    private static int stableCode(String id) {
        return id == null ? 0 : id.hashCode();
    }
}
