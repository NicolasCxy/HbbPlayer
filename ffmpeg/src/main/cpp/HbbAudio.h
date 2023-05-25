//
// Created by DELL on 2023/5/6.
//

#ifndef HBBPLAYER_HBBAUDIO_H
#define HBBPLAYER_HBBAUDIO_H

#include "HbbCallJava.h"
#include "Playstatus.h"
#include "HbbQueue.h"
#include "pthread.h"
#include "SoundTouch.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/time.h"
#include <libswresample/swresample.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
};

using namespace soundtouch;


class HbbAudio {
public:
    Playstatus *playstatus = NULL;
    HbbCallJava *callJava = NULL;
    HbbQueue *queue = NULL;
    uint8_t *buffer = NULL;
    //解码后的pcm数据，之后需要交给soundTouch处理
    uint8_t *out_buffer = NULL;
    int sample_rate;
    int data_size = 0;
    int nb = 0;
    int num = 0;


    //音频处理相关
    SoundTouch *soundTouch = NULL;
    //变数
    float speed = 1.0f;
    //变倍
    float pitch = 1.0f;
    //用来判断是否要取值
    bool finished = true;
    //缓冲区
    SAMPLETYPE *sampleBuffer = NULL;

    //业务相关
    int streamIndex = -1;
    //FFmpeg相关
    AVCodecParameters *codecpar = NULL;
    AVCodecContext *avCodecContext = NULL;
    AVPacket *avPacket = NULL;
    AVFrame *avFrame = NULL;


    //时间相关
    int duration = 0;
    AVRational time_base;
    double clock;//当前播放的时间    准确时间
    double last_tiem; //上一次调用时间
    double now_time;//当前frame时间

    //openSL
    SLObjectItf engineObject = NULL;
    SLEngineItf engineEngine = NULL;

    //混音器
    SLObjectItf outputMixObject = NULL;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;
    SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

    //pcm
    SLObjectItf pcmPlayerObject = NULL;
    SLPlayItf pcmPlayerPlay = NULL;
    SLVolumeItf pcmVolumePlay = NULL;
    SLMuteSoloItf pcmMutePlay = NULL;
    //缓冲器队列接口
    SLAndroidSimpleBufferQueueItf pcmBufferQueue = NULL;

    //锁相关
    pthread_mutex_t make_speed_mutex;
    pthread_mutex_t make_pitch_mutex;

    //线程相关
    pthread_t decodeThread;

public:
    HbbAudio(Playstatus *playstatus, HbbCallJava *callJava, int sample_rate);

    int getCurrentSampleRateForOpensles(int sample_rate);

    void initOpenSLES();

    void play();

    void pause();

    void resume();

    int getSoundTouchData();

    int resampleAudio(void **pcmbuf);

    void setSpeed(float speed);

    void release();


    ~HbbAudio();
};


#endif //HBBPLAYER_HBBAUDIO_H
