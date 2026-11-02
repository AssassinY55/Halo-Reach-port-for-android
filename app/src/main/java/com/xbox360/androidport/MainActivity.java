package com.xbox360.androidport;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {

    private static final int PICK_ISO_REQUEST = 100;
    private TextView statusTextView;
    private SurfaceView surfaceView;

    static {
        System.loadLibrary("xbox360androidport");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        statusTextView = findViewById(R.id.sample_text);
        Button btnSelectIso = findViewById(R.id.btn_select_iso);
        surfaceView = findViewById(R.id.surface_view);

        surfaceView.getHolder().addCallback(this);

        statusTextView.setText(stringFromJNI());
        btnSelectIso.setOnClickListener(v -> openIsoFilePicker());
    }

    private void openIsoFilePicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        startActivityForResult(intent, PICK_ISO_REQUEST);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == PICK_ISO_REQUEST && resultCode == RESULT_OK) {
            if (data != null) {
                Uri uri = data.getData();
                statusTextView.setText("Copying ISO to app storage...");
                
                new Thread(() -> {
                    try {
                        File destFile = new File(getFilesDir(), "reach.iso");
                        InputStream inputStream = getContentResolver().openInputStream(uri);
                        FileOutputStream outputStream = new FileOutputStream(destFile);

                        byte[] buffer = new byte[8192];
                        int length;
                        while ((length = inputStream.read(buffer)) > 0) {
                            outputStream.write(buffer, 0, length);
                        }

                        outputStream.close();
                        inputStream.close();

                        String localPath = destFile.getAbsolutePath();
                        boolean nativeSuccess = nativeInitIso(localPath);

                        runOnUiThread(() -> {
                            if (nativeSuccess) {
                                statusTextView.setText("ISO Verified & Frame Rendered!");
                                Toast.makeText(MainActivity.this, "Engine Canvas Active", Toast.LENGTH_LONG).show();
                                
                                // Trigger our C++ render loop test!
                                nativeRenderFrame();
                            } else {
                                statusTextView.setText("ISO validation failed.");
                            }
                        });

                    } catch (Exception e) {
                        e.printStackTrace();
                        runOnUiThread(() -> statusTextView.setText("Error: " + e.getMessage()));
                    }
                }).start();
            }
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        nativeSetSurface(holder.getSurface());
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        nativeResizeSurface(width, height);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        nativeSetSurface(null);
    }

    public native String stringFromJNI();
    public native boolean nativeInitIso(String isoPath);
    public native void nativeSetSurface(Surface surface);
    public native void nativeResizeSurface(int width, int height);
    public native void nativeRenderFrame();
}
