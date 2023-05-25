//
// Created by DELL on 2023/5/6.
//

#include "HbbAudio.h"

void pcmBufferCallBack(SLAndroidSimpleBufferQueueItf bf, void *context);

HbbAudio::HbbAudio(Playstatus *playStatus, HbbCallJava *callJava, int sample_rate) {



    /**
     * AudioModel 流程
     * 1、初始化soundTouch(音频变倍变速)和openSL(音频低延迟播放)
     * 2、根据openSl的回调监听来塞数据
     *   2.1、去队列读编码数据，解码
     *   2.2、组装成short类型数组，交给soundTouch接管处理
     *   2.3、拿到soundTouch给到OpenSL去播放
     * 3、期间将播放进度给到上层
     * 4、VideoModel 根据音频时间来做同步处理
     * 5、对上层提供音频相关功能：变倍、变速。
     */


    /**
  * 1、初始化soundTouch,为后续重采样、变倍、变速做准备
  * 2、初始化锁
  * 3、初始化队列
  */

    this->playstatus = playStatus;
    this->callJava = callJava;
    this->sample_rate = sample_rate;

    queue = new HbbQueue(playstatus);
    //为后续接收数据做准备
    buffer = static_cast<uint8_t *>(av_malloc(this->sample_rate * 2 * 2));

    //初始化soundTouch
    sampleBuffer = static_cast<SAMPLETYPE *>(malloc(this->sample_rate * 2 * 2));
    soundTouch = new SoundTouch();
    soundTouch->setChannels(2);
    soundTouch->setSampleRate(sample_rate);

    clock = 0;
    last_tiem = 0;

    soundTouch->setPitch(pitch);
    soundTouch->setTempo(speed);

    //初始化锁
    pthread_mutex_init(&make_speed_mutex, NULL);
    pthread_mutex_init(&make_pitch_mutex, NULL);

}

void *playThread(void *data) {
    HbbAudio *hbbAudio = (HbbAudio *) data;
    hbbAudio->initOpenSLES();
    pthread_exit(&hbbAudio->decodeThread);
}

void HbbAudio::play() {
    //开线程，读取数据解码
    pthread_create(&decodeThread, NULL, playThread, this);
}


void HbbAudio::initOpenSLES() {
    //初始化
    SLresult result;
    result = slCreateEngine(&engineObject, 0, 0, 0, 0, 0);
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);

    //第二步，创建混音器
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, mids, mreq);
    (void) result;
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    (void) result;
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                              &outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS == result) {
        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &reverbSettings);
        (void) result;
    }

    // 配置输出对象及其格式
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, 0};


    // 第三步，配置PCM格式信息
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};

    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,//播放pcm格式的数据
            2,//2个声道（立体声）
            static_cast<SLuint32>(getCurrentSampleRateForOpensles(sample_rate)),//44100hz的频率
            SL_PCMSAMPLEFORMAT_FIXED_16,//位数 16位
            SL_PCMSAMPLEFORMAT_FIXED_16,//和位数一致就行
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//立体声（前左前右）
            SL_BYTEORDER_LITTLEENDIAN//结束标志
    };
    SLDataSource slDataSource = {&android_queue, &pcm};


    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_MUTESOLO};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    (*engineEngine)->CreateAudioPlayer(engineEngine, &pcmPlayerObject, &slDataSource, &audioSnk, 2,
                                       ids, req);
    //初始化播放器
    (*pcmPlayerObject)->Realize(pcmPlayerObject, SL_BOOLEAN_FALSE);

//    得到接口后调用  获取Player接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_PLAY, &pcmPlayerPlay);
//    获取声道操作接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_MUTESOLO, &pcmMutePlay);
//   拿控制  播放暂停恢复的句柄
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_VOLUME, &pcmVolumePlay);
//    注册回调缓冲区 获取缓冲队列接口
    (*pcmPlayerObject)->GetInterface(pcmPlayerObject, SL_IID_BUFFERQUEUE, &pcmBufferQueue);
    //缓冲接口回调
    (*pcmBufferQueue)->RegisterCallback(pcmBufferQueue, pcmBufferCallBack, this);
//    获取播放状态接口
    (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PLAYING);
    pcmBufferCallBack(pcmBufferQueue, this);
}


void pcmBufferCallBack(SLAndroidSimpleBufferQueueItf bf, void *context) {
    //c++线程，需要转一下
    HbbAudio *wlAudio = (HbbAudio *) context;
    if (wlAudio != NULL) {
        //读数据，解码
        int buffersize = wlAudio->getSoundTouchData();
        //给OpenSL喂数据
        if (buffersize > 0) {
            wlAudio->clock += buffersize / ((double) wlAudio->sample_rate * 2 * 2);
            LOGD("当前播放时间：%lf, 上一次时长@：%lf",wlAudio->clock, wlAudio->last_tiem)
            if (wlAudio->clock - wlAudio->last_tiem > 1) {
                wlAudio->last_tiem = wlAudio->clock;
                //回调到上层
                wlAudio->callJava->onCallTimeInfo(CHILD_THREAD,wlAudio->clock,wlAudio->duration);
            }

            //塞数据
            (*wlAudio->pcmBufferQueue)->Enqueue(wlAudio->pcmBufferQueue,
                                                (char *) wlAudio->sampleBuffer, buffersize * 2 * 2);
        }
    }
}


int HbbAudio::getCurrentSampleRateForOpensles(int sample_rate) {

    int rate = 0;
    switch (sample_rate) {
        case 8000:
            rate = SL_SAMPLINGRATE_8;
            break;
        case 11025:
            rate = SL_SAMPLINGRATE_11_025;
            break;
        case 12000:
            rate = SL_SAMPLINGRATE_12;
            break;
        case 16000:
            rate = SL_SAMPLINGRATE_16;
            break;
        case 22050:
            rate = SL_SAMPLINGRATE_22_05;
            break;
        case 24000:
            rate = SL_SAMPLINGRATE_24;
            break;
        case 32000:
            rate = SL_SAMPLINGRATE_32;
            break;
        case 44100:
            rate = SL_SAMPLINGRATE_44_1;
            break;
        case 48000:
            rate = SL_SAMPLINGRATE_48;
            break;
        case 64000:
            rate = SL_SAMPLINGRATE_64;
            break;
        case 88200:
            rate = SL_SAMPLINGRATE_88_2;
            break;
        case 96000:
            rate = SL_SAMPLINGRATE_96;
            break;
        case 192000:
            rate = SL_SAMPLINGRATE_192;
            break;
        default:
            rate = SL_SAMPLINGRATE_44_1;
    }
    return rate;
}


int HbbAudio::getSoundTouchData() {
    while (playstatus != NULL && !playstatus->exit) {

//        LOGE("------------------循环---------------------------finished %d", finished)
        out_buffer = NULL;
        if (finished) {
            finished = false;

            data_size = resampleAudio(reinterpret_cast<void **>(&out_buffer));

            //将int数组拼接到short数组中
            if (data_size > 0) {
                for (int i = 0; i < data_size / 2 + 1; i++) {
                    sampleBuffer[i] = out_buffer[i * 2] | (out_buffer[i * 2 + 1] << 8);
                }
                //交给soundTouch去处理
                soundTouch->putSamples(sampleBuffer, nb);
                //除4是因为 short 是 int 的两倍，其次双通道所以在除2  = /4
                num = soundTouch->receiveSamples(sampleBuffer, data_size / 4);

            } else {
                //将剩余数据全部输出
                soundTouch->flush();
            }
        }

        //如果数据已经读完，就将状态设置课读取状态
        if (num == 0) {
            finished = true;
            continue;
        } else {
            //如果sampleBuffer中还有数据，那么就在读取一次。
            if (out_buffer == NULL) {
                num = soundTouch->receiveSamples(sampleBuffer, data_size / 4);
                if (num == 0) {
                    finished = true;
                    continue;
                }
            }
            return num;
        }
    }

    return 0;

}

int HbbAudio::resampleAudio(void **pcmbuf) {
    /**
     * 1、从队列读取数据，解码
     * 2、重采样
     * 3、反馈给soundTouch处理
     */
    while (playstatus != NULL && !playstatus->exit) {
        if (queue->getQueueSize() >= 0) {
            avPacket = av_packet_alloc();
            if (queue->getAvpacket(avPacket) != 0) {
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = NULL;
                continue;
            }

            if (avcodec_send_packet(avCodecContext, avPacket) != 0) {
                if (LOG_DEBUG) {
                    LOGE("avcodec_send_packet error!")
                }
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = nullptr;
                continue;
            }

            avFrame = av_frame_alloc();
            if (avcodec_receive_frame(avCodecContext, avFrame) != 0) {
                if (LOG_DEBUG) {
                    LOGE("avcodec_receive_frame error!")
                }
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = nullptr;
                av_frame_free(&avFrame);
                av_free(avFrame);
                avFrame = nullptr;
                continue;
            }


            //开始重采样
            if (avFrame->channels && avFrame->channel_layout == 0) {
                avFrame->channel_layout = av_get_default_channel_layout(avFrame->channels);
            } else if (avFrame->channels == 0 && avFrame->channel_layout > 0) {
                avFrame->channels = av_get_channel_layout_nb_channels(avFrame->channel_layout);
            }

            SwrContext *swr_ctx;

            swr_ctx = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16,
                                         avFrame->sample_rate, avFrame->channel_layout,
                                         (AVSampleFormat) avFrame->format, avFrame->sample_rate,
                                         NULL, NULL);


            if (!swr_ctx || swr_init(swr_ctx) < 0) {
                av_packet_free(&avPacket);
                av_free(avPacket);
                avPacket = nullptr;
                av_frame_free(&avFrame);
                av_free(avFrame);
                avFrame = nullptr;
                swr_free(&swr_ctx);
                continue;
            }


            //开始转换，返回值为成功转换的数量
            nb = swr_convert(swr_ctx, &buffer, avFrame->nb_samples,
                             (const uint8_t **) avFrame->data, avFrame->nb_samples);

            *pcmbuf = buffer;
            int out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
            data_size = nb * out_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

            //获取当前帧时间
            now_time = avFrame->pts * av_q2d(time_base);
            if (now_time < clock) {
                clock = now_time;
            }

            clock = now_time;

//            if (LOG_DEBUG) {
//                LOGD("dataSize  is %d", data_size);
//            }

            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            swr_free(&swr_ctx);
            break;
        }
    }

    return data_size;
}


void HbbAudio::release() {
    LOGE("释放Auido资源!")

    if (queue->getQueueSize() > 0) {
        queue->clearAvpacket();
        delete (queue);
        queue = nullptr;
    }



    //释放OpenSl
    if (pcmPlayerObject != NULL) {
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_STOPPED);
        (*pcmPlayerObject)->Destroy(pcmPlayerObject);

        pcmPlayerObject = NULL;
        pcmPlayerPlay = NULL;
    }

    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
        outputMixEnvironmentalReverb = NULL;
    }


    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }


    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }


    if (soundTouch != nullptr) {
        soundTouch->clear();
        delete soundTouch;
    }

    if (avCodecContext != nullptr) {
        avcodec_close(avCodecContext);
        avcodec_free_context(&avCodecContext);
        avCodecContext = nullptr;
    }



}

void HbbAudio::setSpeed(float speed) {
    this->speed = speed;
    pthread_mutex_lock(&make_speed_mutex);
    soundTouch->setTempo(speed);
    pthread_mutex_unlock(&make_speed_mutex);
}


void HbbAudio::pause() {
    if (pcmPlayerObject != NULL) {
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PAUSED);
    }
}

void HbbAudio::resume() {
    if (pcmPlayerObject != NULL) {
        (*pcmPlayerPlay)->SetPlayState(pcmPlayerPlay, SL_PLAYSTATE_PLAYING);
    }
}


HbbAudio::~HbbAudio() {

}









