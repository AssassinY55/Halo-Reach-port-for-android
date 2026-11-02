package com.xbox360.androidport;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

public class MainActivity extends AppCompatActivity {

    private static final int PICK_ISO_REQUEST = 100;
    private TextView statusTextView;

    static {
        System.loadLibrary("xbox360androidport");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        statusTextView = findViewById(R.id.sample_text);
        Button btnSelectIso = findViewById(R.id.btn_select_iso);

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
                statusTextView.setText("Copying ISO to app storage...\nPlease wait.");
                
                // Copy the ISO in a background thread so the UI doesn't freeze
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

                        // Pass the local file path down to C++
                        String localPath = destFile.getAbsolutePath();
                        boolean nativeSuccess = nativeInitIso(localPath);

                        runOnUiThread(() -> {
                            if (nativeSuccess) {
                                statusTextView.setText("ISO Loaded & Verified by C++!\nPath: " + localPath);
                                Toast.makeText(MainActivity.this, "Ready to launch engine!", Toast.LENGTH_LONG).show();
                            } else {
                                statusTextView.setText("Copied, but C++ init failed.");
                            }
                        });

                    } catch (Exception e) {
                        e.printStackTrace();
                        runOnUiThread(() -> statusTextView.setText("Error copying ISO: " + e.getMessage()));
                    }
                }).start();
            }
        }
    }

    public native String stringFromJNI();
    public native boolean nativeInitIso(String isoPath);
}
