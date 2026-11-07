package org.dudetronics.droidstar;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Owns the system file picker. Qt's activity drops the result of a document
 * intent, which left Export pressed and Restore unable to start.
 *
 * Export matches the Windows save dialog as closely as SAF allows: create a
 * new file, or pick an existing one and confirm before replacing it.
 */
public class ConfigTransferActivity extends Activity {
    public static final String EXTRA_MODE = "mode";
    public static final String EXTRA_PATH = "path";
    private static final int REQUEST = 1;
    private boolean mStarted = false;
    private boolean mExportReplaceExisting = false;
    private Uri mPendingOverwriteUri = null;

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
        if ("import".equals(mode)) {
            startOpenDocument(false);
            return;
        }
        new AlertDialog.Builder(this)
                .setTitle("Export settings")
                .setMessage("Save a new file, or choose an existing settings file to replace?")
                .setPositiveButton("New file", (d, which) -> startCreateDocument())
                .setNeutralButton("Replace existing", (d, which) -> startOpenDocument(true))
                .setNegativeButton("Cancel", (d, which) -> finishWithResult(false, "cancel"))
                .setOnCancelListener(d -> finishWithResult(false, "cancel"))
                .show();
    }

    private void startCreateDocument() {
        mExportReplaceExisting = false;
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        intent.putExtra(Intent.EXTRA_TITLE, "DroidStar-config.conf");
        intent.putExtra(Intent.EXTRA_MIME_TYPES, new String[] {
                "application/octet-stream",
                "text/plain",
                "*/*"
        });
        startActivityForResult(intent, REQUEST);
    }

    private void startOpenDocument(boolean forExportOverwrite) {
        mExportReplaceExisting = forExportOverwrite;
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        if (forExportOverwrite) {
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
                    | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
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
        if (requestCode != REQUEST || resultCode != RESULT_OK || data == null || data.getData() == null) {
            finishWithResult(false, "cancel");
            return;
        }

        String mode = getIntent().getStringExtra(EXTRA_MODE);
        String path = getIntent().getStringExtra(EXTRA_PATH);
        Uri uri = data.getData();
        if ("import".equals(mode)) {
            finishWithResult(copyUriToFile(uri, path), "import");
            return;
        }

        // Export: confirm before replacing an existing file (picked explicitly, or
        // CREATE_DOCUMENT returned a URI that already has content).
        if (mExportReplaceExisting || uriHasContent(uri)) {
            mPendingOverwriteUri = uri;
            String name = displayName(uri);
            String message = (name != null && !name.isEmpty())
                    ? ("Replace \"" + name + "\"?")
                    : "Replace the existing file?";
            new AlertDialog.Builder(this)
                    .setTitle("Overwrite file")
                    .setMessage(message)
                    .setPositiveButton("Replace", (d, which) -> {
                        Uri target = mPendingOverwriteUri;
                        mPendingOverwriteUri = null;
                        finishWithResult(copyFileToUri(path, target), "export");
                    })
                    .setNegativeButton("Cancel", (d, which) -> {
                        mPendingOverwriteUri = null;
                        finishWithResult(false, "cancel");
                    })
                    .setOnCancelListener(d -> {
                        mPendingOverwriteUri = null;
                        finishWithResult(false, "cancel");
                    })
                    .show();
            return;
        }

        finishWithResult(copyFileToUri(path, uri), "export");
    }

    private void finishWithResult(boolean ok, String kind) {
        try {
            PlatformBridge.nativeConfigTransferDone(ok, kind);
        } catch (UnsatisfiedLinkError ignored) {
        }
        finish();
    }

    private boolean uriHasContent(Uri uri) {
        try (Cursor cursor = getContentResolver().query(uri,
                new String[] { OpenableColumns.SIZE }, null, null, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                int idx = cursor.getColumnIndex(OpenableColumns.SIZE);
                if (idx >= 0 && !cursor.isNull(idx) && cursor.getLong(idx) > 0) {
                    return true;
                }
            }
        } catch (Exception ignored) {
        }
        try (InputStream in = getContentResolver().openInputStream(uri)) {
            return in != null && in.read() != -1;
        } catch (Exception ignored) {
            return false;
        }
    }

    private String displayName(Uri uri) {
        try (Cursor cursor = getContentResolver().query(uri,
                new String[] { OpenableColumns.DISPLAY_NAME }, null, null, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                int idx = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (idx >= 0) {
                    return cursor.getString(idx);
                }
            }
        } catch (Exception ignored) {
        }
        return null;
    }

    private boolean copyFileToUri(String path, Uri uri) {
        if (uri == null) {
            return false;
        }
        try (InputStream in = new FileInputStream(path);
             OutputStream out = getContentResolver().openOutputStream(uri, "wt")) {
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
