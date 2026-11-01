
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    private static final int PICK_ISO_REQUEST = 100;
    private TextView statusTextView;

    // Load the native library on startup
    static {
        System.loadLibrary("xbox360androidport");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main.xml assuming layout resource lookup... wait, let's use R.layout.activity_main)
      
