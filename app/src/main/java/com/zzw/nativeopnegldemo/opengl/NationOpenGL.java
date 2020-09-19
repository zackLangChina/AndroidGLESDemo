package com.zzw.nativeopnegldemo.opengl;

import android.view.Surface;

public class NationOpenGL {

    static {
        System.loadLibrary("native-lib");
    }

    public native void nativeSurfaceCreate(Surface surface);

    public native void nativeSurfaceChanged(int width, int height);

    public native void nativeSurfaceDestroyed();

}