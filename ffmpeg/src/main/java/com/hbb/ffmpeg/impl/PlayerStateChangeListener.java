package com.hbb.ffmpeg.impl;

/**
 * 播放状态变更监听
 */
public interface PlayerStateChangeListener {
    void onBeReady();   //准备好了

    void onCurrentTime(int currentTime,int totalTime);

    void onComplete();

    void onStop(); //停止播放

}
