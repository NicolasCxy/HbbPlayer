package com.hbb.ffmpeg.palyer;

import android.util.Log;

import com.hbb.ffmpeg.NativeCallBack;
import com.hbb.ffmpeg.impl.PlayerStateChangeListener;

public class HbbPlayer {
    private static final String TAG = "HbbPlayer";

    static {
        System.loadLibrary("cxyPlayer");
    }


    public void prepare(String path) {
        Log.i(TAG, "preparePath: " + path);
        n_prepare(path);
    }

    public void start() {
        n_start();
    }

    public void setChangeListener(PlayerStateChangeListener changeListener) {
        this.changeListener = changeListener;
    }

    PlayerStateChangeListener changeListener;


    //==========================================回调区==============================================
    @NativeCallBack
    private void onBeReady() {
        //回调通知上层
        if (null != changeListener) {
            changeListener.onBeReady();
        }
        Log.w(TAG, "onBeReady -> 加载成功！ ");
    }

    public void onCallRenderYUV(int width, int height, byte[] y, byte[] u, byte[] v) {

    }


    //==========================================native函数区==============================================

    private native void n_prepare(String path);

    private native void n_start();


}
