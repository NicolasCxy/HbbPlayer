//
// Created by DELL on 2023/4/27.
//

#ifndef HBBPLAYER_HBBVIDEO_H
#define HBBPLAYER_HBBVIDEO_H

#include "HbbCallJava.h"
#include "Playstatus.h"
#include "pthread.h"
#include "HbbQueue.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/time.h"
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
};


class HbbVideo {

public:
    Playstatus *playstatus = NULL;
    HbbCallJava *callJava = NULL;
    int streamIndex = -1;
    HbbQueue *queue = NULL;

    //锁、线程
    pthread_mutex_t codecMutex;
    pthread_t* decodeThread;

    //FFmpeg相关
    AVCodecParameters *codecpar = NULL;
    AVCodecContext *avCodecContext = NULL;
    pthread_t thread_play;

    //时间相关
    int duration = 0;
    AVRational time_base;
    int defaultDelayTime;   //帧读取频率

public:
    HbbVideo(Playstatus *playstatus,HbbCallJava *callJava);

    void play();

    void decode();

    ~HbbVideo();
};


#endif //HBBPLAYER_HBBVIDEO_H
