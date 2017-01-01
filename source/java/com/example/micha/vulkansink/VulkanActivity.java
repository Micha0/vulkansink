package com.example.micha.vulkansink;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;

public class VulkanActivity extends Activity implements SurfaceHolder.Callback
{
    private SurfaceView m_MainView;
    private AssetManager assetManagerInstance;

    public static native void nativeOnStart();
    public static native void nativeOnResume();
    public static native void nativeOnPause();
    public static native void nativeOnStop();
    public static native void nativeOnRestart();
    public static native void nativeOnDestroy();
    public static native void nativeOnInput(int type, float x, float y);
    public static native void nativeSetSurface(Surface surface);
    public static native void nativeSetAssetManager(AssetManager assetManagerInstance);
    public static native void nativeOnConfigurationChanged();
    public static native void nativeOnWindowFocusChanged(boolean hasFocus);
//    public static native void nativeDoFrame(long frameTimeNanos);

    @SuppressLint("ClickableViewAccessibility")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //this.setContentView();

        int SDK_INT = android.os.Build.VERSION.SDK_INT;
        if(SDK_INT >= 19) {
            setFullscreen();
            View decorView = getWindow().getDecorView();
            decorView.setOnSystemUiVisibilityChangeListener
                (new View.OnSystemUiVisibilityChangeListener() {
                    @Override
                    public void onSystemUiVisibilityChange(int visibility) {
                        setFullscreen();
                    }
                });
        }

        m_MainView = new SurfaceView(this);
        m_MainView.getHolder().addCallback(this);
        m_MainView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, MotionEvent motionEvent) {
                if (motionEvent.getAction() == MotionEvent.ACTION_DOWN || motionEvent.getAction() == MotionEvent.ACTION_UP || motionEvent.getAction() == MotionEvent.ACTION_MOVE)
                    nativeOnInput(motionEvent.getAction(), motionEvent.getX(), motionEvent.getY());
                return true;
            }
        });

        addContentView(m_MainView, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.FILL_PARENT));

        assetManagerInstance = getResources().getAssets();
        nativeSetAssetManager(assetManagerInstance);

        nativeOnStart();
    }

    @Override
    public void onStart()
    {
        super.onStart();

        nativeOnStart();
    }

    @Override
    public void onPause()
    {
        super.onPause();

        nativeOnPause();
    }

    @Override
    public void onResume()
    {
        super.onResume();

        //go fullscreen
        int SDK_INT = android.os.Build.VERSION.SDK_INT;
        if(SDK_INT >= 11 && SDK_INT < 14) {
            getWindow().getDecorView().setSystemUiVisibility(View.STATUS_BAR_HIDDEN);
        } else if(SDK_INT >= 14 && SDK_INT < 19) {
            getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN |
                    View.SYSTEM_UI_FLAG_LOW_PROFILE);
        } else if(SDK_INT >= 19) {
            setFullscreen();
        }

        nativeOnResume();
    }

    @Override
    public void onStop()
    {
        super.onStop();

        nativeOnStop();
    }

    @Override
    public void onRestart()
    {
        super.onRestart();

        nativeOnRestart();
    }

    @Override
    public void onDestroy()
    {
        super.onDestroy();

        nativeOnDestroy();
    }

    @Override
    public void onConfigurationChanged(Configuration config)
    {
        super.onConfigurationChanged(config);

        nativeOnConfigurationChanged();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus)
    {
        super.onWindowFocusChanged(hasFocus);

        nativeOnWindowFocusChanged(hasFocus);
    }

    //SurfaceHolder.Callback
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        Log.d("GLS.Java", "Surface Changed");

        nativeSetSurface(holder.getSurface());
    }

    public void surfaceCreated(SurfaceHolder holder) {
        Log.d("GLS.Java", "Surface Created");
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.d("GLS.Java", "Surface Destroyed");

        nativeSetSurface(null);
    }

    void setFullscreen() {
        View decorView = getWindow().getDecorView();
        decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
    }

    static {
        System.loadLibrary("vulkansink");
    }
}
