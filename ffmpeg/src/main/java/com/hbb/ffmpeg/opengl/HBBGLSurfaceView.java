package com.hbb.ffmpeg.opengl;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;

public class HBBGLSurfaceView extends GLSurfaceView {

    private HBBRender hbbRender;

    public HBBGLSurfaceView(Context context) {
        super(context);
    }

    public HBBGLSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setEGLContextClientVersion(2);
        hbbRender = new HBBRender(context);
        setRenderer(hbbRender);
        setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
    }

    public void setYUVData(int width, int height, byte[] y, byte[] u, byte[] v) {
        if(hbbRender != null){
            hbbRender.showData(width,height,y,u,v);
            requestRender();
        }

    }
}
