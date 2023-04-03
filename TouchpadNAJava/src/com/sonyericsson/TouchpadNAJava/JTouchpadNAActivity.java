package ${YYAndroidPackageName};

import ${YYAndroidPackageName}.R;
import ${YYAndroidPackageName}.RunnerActivity;
import com.yoyogames.runner.RunnerJNILib;
import android.util.Log;
import android.content.Intent;
import android.app.Activity;
import com.sonyericsson.TouchpadNAJava.TouchpadNAActivity;
import java.io.StringWriter;
import java.io.PrintWriter;

// Import the native functions declared in HelloNativeAndroid.java
import static com.sonyericsson.TouchpadNAJava.TouchpadNAActivity.TestLink;
import static com.sonyericsson.TouchpadNAJava.TouchpadNAActivity.GetPosX;
import static com.sonyericsson.TouchpadNAJava.TouchpadNAActivity.GetPosY;

// Class name need to match the one you used when creating the extension in GMS
public class JTouchpadNAActivity extends RunnerActivity
{
    public void JInit()
    {
        Log.i("yoyo","Init function");
        
        try {
            //  Block of code to try
            Intent myIntent = new Intent(RunnerActivity.CurrentActivity.getApplicationContext(), TouchpadNAActivity.class);
            RunnerActivity.CurrentActivity.startActivity(myIntent);
        }
        catch(Exception e) {
            //  Block of code to handle errors
            StringWriter sw = new StringWriter();
            e.printStackTrace(new PrintWriter(sw));
            String exceptionAsString = sw.toString();
            Log.i("yoyo",exceptionAsString);
        }
        
        Log.i("yoyo","Created intent!!!!!!!!!!!");
    }
    public double JTestLink()
    {
        return TestLink();
    }

    public double JGetLink()
    {
        return 69.0;
    }

    public double JGetPosX()
    {
        return GetPosX();
    }

    public double JGetPosY()
    {
        return GetPosY();
    }
}