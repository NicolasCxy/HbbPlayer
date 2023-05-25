//
// Created by DELL on 2023/4/27.
//

#ifndef HBBPLAYER_HBBVIDEO_H
#define HBBPLAYER_HBBVIDEO_H

#include "HbbCallJava.h"
#include "Playstatus.h"
#include "pthread.h"
#include "HbbQueue.h"
#include <chrono>
#include <thread>
#include "HbbAudio.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/time.h"
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
};


using namespace std::chrono;
using namespace std::this_thread;

class HbbVideo {

public:
    Playstatus *playstatus = NULL;
    HbbCallJava *callJava = NULL;
    int streamIndex = -1;
    HbbQueue *queue = NULL;

    //锁、线程
    pthread_mutex_t codecMutex;
    pthread_t decodeThread;

    //FFmpeg相关
    AVCodecParameters *codecpar = NULL;
    AVCodecContext *avCodecContext = NULL;
    pthread_t thread_play;

    //时间相关
    int duration = 0;
    AVRational time_base;
    double clock;//当前播放的时间    准确时间
    double last_tiem; //上一次调用时间
    double delayTime = 0;
    double defaultDelayTime = 0.04;   //帧读取频率
    microseconds kFrameTime;   //帧读取频率 （微秒）
    steady_clock::time_point next_frame_time;

    HbbAudio *audio = NULL;

public:
    HbbVideo(Playstatus *playstatus,HbbCallJava *callJava);

    void play();

    void decode();

    void sleepProcess();

    double getFrameDiffTime(AVFrame *avFrame);

    double getDelayTime(double diff);

    void release();

    ~HbbVideo();
};


#endif //HBBPLAYER_HBBVIDEO_H
