package com.xbox360.androidport;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;

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

        // Update text from C++ native library
        statusTextView.setText(stringFromJNI());

        // When the button is clicked, open the phone's file picker
        btnSelectIso.setOnClickListener(v -> openIsoFilePicker());
    }

    private void openIsoFilePicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*"); // Allows selecting ISO/bin files
        startActivityForResult(intent, PICK_ISO_REQUEST);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == PICK_ISO_REQUEST && resultCode == RESULT_OK) {
            if (data != null) {
                Uri uri = data.getData();
                statusTextView.setText("ISO Selected:\n" + uri.getPath());
                Toast.makeText(this, "ISO Loaded Successfully!", Toast.LENGTH_LONG).show();
            }
        }
    }

    public native String stringFromJNI();
}
