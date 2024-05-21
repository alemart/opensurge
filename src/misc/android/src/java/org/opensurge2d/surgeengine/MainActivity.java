/*
 * Open Surge Engine
 * MainActivity.java - Main Activity
 * Copyright (C) 2008-2024 Alexandre Martins <alemartf@gmail.com>
 * http://opensurge2d.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package org.opensurge2d.surgeengine;

import org.liballeg.android.AllegroActivity;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;
import android.view.Window;
import android.widget.Toast;
import java.io.File;

public class MainActivity extends AllegroActivity
{
    private static final String TAG = "opensurge";
    
    static void loadLibrary(String name)
    {
        try {
            Log.d("loadLibrary", name);
            System.loadLibrary(name);
        }
        catch(UnsatisfiedLinkError e) {
            Log.d("loadLibrary", name + " FAILED");
        }
    }

    static
    {
        loadLibrary("physfs");
        loadLibrary("allegro");
        loadLibrary("allegro_primitives");
        loadLibrary("allegro_image");
        loadLibrary("allegro_font");
        loadLibrary("allegro_ttf");
        loadLibrary("allegro_audio");
        loadLibrary("allegro_acodec");
        loadLibrary("allegro_color");
        loadLibrary("allegro_dialog");
        loadLibrary("allegro_memfile");
        loadLibrary("allegro_physfs");
    }

    public MainActivity()
    {
        super("libopensurge.so");
    }

    public void openWebPage(String url)
    {
        Log.d(TAG, "Will open URL " + url);

        if(isTVDevice()) {
            // "For web content, the app may only use WebView components. The app may not attempt to launch a web browser app."
            // https://developer.android.com/docs/quality-guidelines/tv-app-quality
            Log.e(TAG, "Can't open URLs in TVs");
            showToast("Can't open URLs in TVs");
            return;
        }

        if(url.startsWith("https://") || url.startsWith("http://") || url.startsWith("mailto:")) {
            Uri uri = Uri.parse(url);
            Intent intent = new Intent(Intent.ACTION_VIEW, uri);

            if(url.startsWith("https://play.google.com/store/apps/details?id=")) {
                // https://developer.android.com/distribute/marketing-tools/linking-to-google-play#android-app
                intent.setPackage("com.android.vending");
            }

            startActivity(intent);
        }
        else {
            Log.e(TAG, "Can't open URL " + url);
            showToast("Can't open URL");
        }
    }

    public void shareText(String text)
    {
        Intent sendIntent = new Intent(Intent.ACTION_SEND);
        sendIntent.setType("text/plain");
        sendIntent.putExtra(Intent.EXTRA_TEXT, text);

        Intent shareIntent = Intent.createChooser(sendIntent, "Share");
        startActivity(shareIntent);
    }
    
    public boolean isTVDevice()
    {
        return getPackageManager().hasSystemFeature(PackageManager.FEATURE_LEANBACK);
    }

    public void showToast(String text)
    {
        Toast toast = Toast.makeText(this, text, Toast.LENGTH_SHORT);
        toast.show();
    }

    public boolean mkdir(String path)
    {
        Log.d(TAG, "Will call mkdir(\"" + path + "\")");

        try {
            File file = new File(path);
            if(!file.mkdir())
                throw new RuntimeException("mkdir failed");
        }
        catch(Exception e) {
            Log.d(TAG, "Can't mkdir(\"" + path + "\"): " + e.getMessage());
            return false;
        }
        
        return true;
    }

    @Override
    public void onCreate(Bundle savedInstanceBundle)
    {
        super.onCreate(savedInstanceBundle);
        Window win = this.getWindow();

        // Android TV: auto low latency mode
        if(Build.VERSION.SDK_INT >= 30)
            win.setPreferMinimalPostProcessing(true);
    }

    @Override public void onPause() { Log.d(TAG, "<onPause>"); super.onPause(); Log.d(TAG, "</onPause>"); }
    @Override public void onResume() { Log.d(TAG, "<onResume>"); super.onResume(); Log.d(TAG, "</onResume>"); }
    @Override public void onStop() { Log.d(TAG, "<onStop>"); super.onStop(); Log.d(TAG, "</onStop>"); }
    @Override public void onStart() { Log.d(TAG, "<onStart>"); super.onStart(); Log.d(TAG, "</onStart>"); }
    @Override public void onRestart() { Log.d(TAG, "<onRestart>"); super.onRestart(); Log.d(TAG, "</onRestart>"); }
}
