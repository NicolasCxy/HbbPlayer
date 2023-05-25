//
// Created by DELL on 2023/4/27.
//

#ifndef HBBPLAYER_FFMPEGCONTROL_H
#define HBBPLAYER_FFMPEGCONTROL_H


#include "HbbCallJava.h"
#include "Playstatus.h"
#include "pthread.h"
#include "HbbVideo.h"
#include "HbbAudio.h"
#include <chrono>

extern "C"
{
#include <libavutil/time.h>
#include "libavformat/avformat.h"
}

using namespace std::chrono;

class FFmpegControl {
public:
    //业务
    HbbCallJava *callJava = NULL;
    Playstatus *playStatus = NULL;
    const char *url = NULL;
    HbbVideo *mVideo = NULL;
    HbbAudio *mAudio = NULL;
    //ffmpeg
    AVFormatContext *pFormatCtx = NULL;

    int duration = 0;

    bool actionStop = false;

    //锁
    pthread_t initThread;
    pthread_mutex_t seek_mutex;
    pthread_mutex_t init_mutex;


public:
    FFmpegControl(HbbCallJava *callJava, Playstatus *playStatus);

    ~FFmpegControl();

    void prepare();

    void initFFmpegMode();

    void start();

    void seek(int64_t second);

    void pause();

    void resume();

    void stop();

    void setUrl(const char *url);

    void speed(float speed);

    void release();

    void releaseResources();

    void getCodecContext(AVCodecParameters *pParameters, AVCodecContext **pContext);

};


#endif //HBBPLAYER_FFMPEGCONTROL_H
