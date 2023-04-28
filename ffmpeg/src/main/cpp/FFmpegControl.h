//
// Created by DELL on 2023/4/27.
//

#ifndef HBBPLAYER_FFMPEGCONTROL_H
#define HBBPLAYER_FFMPEGCONTROL_H


#include "HbbCallJava.h"
#include "Playstatus.h"
#include "pthread.h"
#include "HbbVideo.h"

extern "C"
{
#include <libavutil/time.h>
#include "libavformat/avformat.h"
}


class FFmpegControl {
public:
    //业务
    HbbCallJava *callJava = NULL;
    Playstatus *playStatus = NULL;
    const char *url = NULL;
    HbbVideo *mVideo = NULL;
    //ffmpeg
    AVFormatContext *pFormatCtx = NULL;

    //锁
    pthread_t initThread;
    pthread_mutex_t seek_mutex;
    pthread_mutex_t init_mutex;


public:
    FFmpegControl(HbbCallJava *callJava, Playstatus *playStatus, const char *url);

    ~FFmpegControl();

    void prepare();

    void initFFmpegMode();

    void start();

    void getCodecContext(AVCodecParameters *pParameters, AVCodecContext **pContext);

};


#endif //HBBPLAYER_FFMPEGCONTROL_H
