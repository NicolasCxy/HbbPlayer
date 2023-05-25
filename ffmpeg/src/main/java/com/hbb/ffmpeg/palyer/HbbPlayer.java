package com.hbb.ffmpeg.palyer;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import com.hbb.ffmpeg.NativeCallBack;
import com.hbb.ffmpeg.impl.PlayerStateChangeListener;
import com.hbb.ffmpeg.opengl.HBBGLSurfaceView;

public class HbbPlayer {
    private static final String TAG = "HbbPlayer";

    private HBBGLSurfaceView mHbbGlView;


    private int duration = 0;

    static {
        System.loadLibrary("cxyPlayer");
    }

    public void prepare(String path) {
        Log.i(TAG, "preparePath: " + path);
        n_prepare(path);
    }

    public void start() {
        new Thread(() -> n_start()).start();

    }


    public void seek(int second) {
        n_seek(second);
    }

    public void stop() {
        n_stop();
    }

    public void pause() {
        n_pause();
    }

    public void resume() {
        n_resume();
    }

    public void speed(float speed) {
        n_speed(speed);
    }

    public int getDuration() {
        return duration;
    }

    //==========================================回调区==============================================

    PlayerStateChangeListener changeListener;

    public void setChangeListener(PlayerStateChangeListener changeListener) {
        this.changeListener = changeListener;
    }


    @NativeCallBack
    private void onBeReady() {
        //回调通知上层
        if (null != changeListener) {
            changeListener.onBeReady();
        }
        Log.w(TAG, "onBeReady -> 加载成功！ ");
    }

    @NativeCallBack
    public void onCallRenderYUV(int width, int height, byte[] y, byte[] u, byte[] v) {
        if (this.mHbbGlView != null) {
            this.mHbbGlView.setYUVData(width, height, y, u, v);
        }
//        Log.i(TAG, "onCallRenderYUV: " + width + ",height:" + height);

    }

    @NativeCallBack
    public void onCallTimeInfo(int currentTime, int totalTime) {
        Log.d(TAG, "onCallTimeInfo@: " + currentTime + ",totalTime: " + totalTime);
        if (null != changeListener) {
            this.duration = totalTime;
            changeListener.onCurrentTime(currentTime, totalTime);
        }
    }

    private Handler mHandler = new Handler(Looper.myLooper());
    @NativeCallBack
    public void onPlaybackCompleted(){
        Log.d(TAG, "onPlaybackCompleted!!！");
        if (null != changeListener) {
            changeListener.onComplete();
        }
    }

    @NativeCallBack
    public void onPlaybackStop(){
        Log.d(TAG, "onPlaybackStop！！");
        if (null != changeListener) {
            changeListener.onStop();
        }
    }



    //==========================================native函数区==============================================

    private native void n_prepare(String path);

    private native void n_start();

    private native void n_seek(int second);

    private native void n_stop();

    private native void n_pause();

    private native void n_resume();

    public native void n_speed(float speed);


    public void setSurfaceView(HBBGLSurfaceView hbbGlView) {
        this.mHbbGlView = hbbGlView;
    }



}
