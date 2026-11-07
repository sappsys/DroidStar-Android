package org.dudetronics.droidstar;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Owns the system file picker. Qt's activity drops the result of a document
 * intent, which left Export pressed and Restore unable to start.
 */
public class ConfigTransferActivity extends Activity {
    public static final String EXTRA_MODE = "mode";
    public static final String EXTRA_PATH = "path";
    private static final int REQUEST = 1;
    private boolean mStarted = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // This screen must never be what the launcher restores. A restored copy
        // sits blank and the app looks like it will not open.
        if (savedInstanceState != null || isTaskRoot()) {
            Intent launch = getPackageManager().getLaunchIntentForPackage(getPackageName());
            if (launch != null) {
                launch.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
                startActivity(launch);
            }
            finish();
            return;
        }
        mStarted = true;
        String mode = getIntent().getStringExtra(EXTRA_MODE);
        boolean importing = "import".equals(mode);
        Intent intent = new Intent(importing ? Intent.ACTION_OPEN_DOCUMENT : Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType(importing ? "*/*" : "application/octet-stream");
        if (!importing) {
            intent.putExtra(Intent.EXTRA_TITLE, "DroidStar-config.conf");
        }
        startActivityForResult(intent, REQUEST);
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putBoolean("started", mStarted);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        boolean ok = false;
        String kind = "cancel";
        if (requestCode == REQUEST && resultCode == RESULT_OK && data != null && data.getData() != null) {
            String mode = getIntent().getStringExtra(EXTRA_MODE);
            String path = getIntent().getStringExtra(EXTRA_PATH);
            kind = "import".equals(mode) ? "import" : "export";
            ok = "import".equals(mode)
                    ? copyUriToFile(data.getData(), path)
                    : copyFileToUri(path, data.getData());
        }
        try {
            PlatformBridge.nativeConfigTransferDone(ok, kind);
        } catch (UnsatisfiedLinkError ignored) {
        }
        finish();
    }

    private boolean copyFileToUri(String path, Uri uri) {
        try (InputStream in = new FileInputStream(path);
             OutputStream out = getContentResolver().openOutputStream(uri)) {
            if (out == null) {
                return false;
            }
            byte[] buf = new byte[8192];
            int n;
            while ((n = in.read(buf)) >= 0) {
                if (n > 0) {
                    out.write(buf, 0, n);
                }
            }
            out.flush();
            return true;
        } catch (Exception ignored) {
            return false;
        }
    }

    private boolean copyUriToFile(Uri uri, String path) {
        try (InputStream in = getContentResolver().openInputStream(uri);
             OutputStream out = new FileOutputStream(path)) {
            if (in == null) {
                return false;
            }
            byte[] buf = new byte[8192];
            int total = 0;
            int n;
            while ((n = in.read(buf)) >= 0) {
                total += n;
                if (total > 2 * 1024 * 1024) {
                    return false;
                }
                if (n > 0) {
                    out.write(buf, 0, n);
                }
            }
            out.flush();
            return total > 0;
        } catch (Exception ignored) {
            return false;
        }
    }
}
