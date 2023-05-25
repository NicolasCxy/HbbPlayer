//
// Created by DELL on 2023/4/27.
//

#include "FFmpegControl.h"


FFmpegControl::FFmpegControl(HbbCallJava *callJava, Playstatus *playStatus) {
    this->callJava = callJava;
    this->playStatus = playStatus;

    pthread_mutex_init(&seek_mutex, NULL);
    pthread_mutex_init(&init_mutex, NULL);
}

void FFmpegControl::setUrl(const char *url) {
    this->url = url;

    LOGD("ffmpegSetUrl -> 设置Url :%s", url);
}

void *initFFmpeg(void *data) {
    FFmpegControl *ffCtl = (FFmpegControl *) data;
    ffCtl->initFFmpegMode();
    pthread_exit(&ffCtl->initThread);

}

void FFmpegControl::prepare() {
    pthread_create(&initThread, NULL, initFFmpeg, this);
}


void FFmpegControl::initFFmpegMode() {
    pthread_mutex_lock(&init_mutex);
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    LOGD("ffmpegInit -> ptah！: %s", url);
    if (avformat_open_input(&pFormatCtx, url, NULL, NULL) != 0) {
        if (LOG_DEBUG) {
            LOGE("can not open url :%s", url);
        }
        return;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        // 查找视频流信息失败
        if (LOG_DEBUG) {
            LOGE("can not find stream from :%s", url);
        }
        return;
    }

    // 查找第一个视频流
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {

        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {     //音频轨
            if (mAudio == nullptr) {
                mAudio = new HbbAudio(playStatus, callJava,
                                      pFormatCtx->streams[i]->codecpar->sample_rate);

                mAudio->streamIndex = i;
                mAudio->duration = pFormatCtx->duration / AV_TIME_BASE;
                mAudio->codecpar = pFormatCtx->streams[i]->codecpar;
                mAudio->time_base = pFormatCtx->streams[i]->time_base;

                duration = mVideo->duration;
            }
        } else if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) { //视频轨
            if (mVideo == nullptr) {
                mVideo = new HbbVideo(playStatus, callJava);
            }

            mVideo->streamIndex = i;
            mVideo->duration = pFormatCtx->duration / AV_TIME_BASE;
            mVideo->codecpar = pFormatCtx->streams[i]->codecpar;
            mVideo->time_base = pFormatCtx->streams[i]->time_base;



//           int frame = av_q2d(pFormatCtx->streams[i]->avg_frame_rate);
            //找出帧数
            int num = pFormatCtx->streams[i]->avg_frame_rate.num;
            int den = pFormatCtx->streams[i]->avg_frame_rate.den;
            if (num != 0 && den != 0) {
                int fps = num / den;
                LOGE("帧数：%d", fps);
                mVideo->defaultDelayTime = 1.0 / fps;
                mVideo->kFrameTime = microseconds(1000000 / fps);
                LOGE("默认延迟毫秒：%ld", mVideo->kFrameTime.count())
            }
        }
    }

    if (mAudio != NULL) {
        getCodecContext(mAudio->codecpar, &mAudio->avCodecContext);
    }

    if (mVideo != NULL) {
        getCodecContext(mVideo->codecpar, &mVideo->avCodecContext);
    }

    callJava->onCallParpared(CHILD_THREAD);
    pthread_mutex_unlock(&init_mutex);

}

/**
 * 根据编码参数打开编码器并获取AVCodecContext
 * @param codecpar
 * @param avCodecContext
 */
void FFmpegControl::getCodecContext(AVCodecParameters *codecpar, AVCodecContext **avCodecContext) {

    const AVCodec *dec = avcodec_find_decoder(codecpar->codec_id);
    if (!dec) {
        if (LOG_DEBUG) {
            LOGE("can not find decoder");
        }

        playStatus->exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }

// 初始化解码器
    *avCodecContext = avcodec_alloc_context3(dec);
    if (!*avCodecContext) {
        if (LOG_DEBUG) {
            LOGE("can not alloc new decodecctx");
        }
        playStatus->exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }

    if (avcodec_parameters_to_context(*avCodecContext, codecpar) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not fill decodecctx");
        }
        playStatus->exit = true;
        pthread_mutex_unlock(&init_mutex);

        return;
    }

    if (avcodec_open2(*avCodecContext, dec, 0) != 0) {
        if (LOG_DEBUG) {
            LOGE("cant not open avCodec!");
        }
        playStatus->exit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }

}

void FFmpegControl::start() {
    playStatus->exit = false;

    if (mAudio == NULL) {
        if (LOG_DEBUG) {
            LOGE("audio is null");
            return;
        }
    }


    if (mVideo == NULL) {
        if (LOG_DEBUG) {
            LOGE("video is null");
            return;
        }
    }

    mAudio->play();

    mVideo->play();

    //video持有Audio引用，为了做唇音同步
    mVideo->audio = mAudio;

    while (playStatus != nullptr && !playStatus->exit) {
        if (playStatus->seek) {
            continue;
        }

        if (playStatus->pause) {
            continue;
        }

        if (mAudio->queue->getQueueSize() > 30) {
            //队列满了等待10毫秒在继续操作
//            av_usleep(1000 * 10);
            continue;
        }

        AVPacket *avPacket = av_packet_alloc();
        if (av_read_frame(pFormatCtx, avPacket) == 0) {
//            LOGD("读取读取读取@")
            if (avPacket->stream_index == mAudio->streamIndex) {
                mAudio->queue->putAvpacket(avPacket);
            } else if (avPacket->stream_index == mVideo->streamIndex) {      //视频模块
                mVideo->queue->putAvpacket(avPacket);
            } else {
                av_packet_free(&avPacket);
                av_free(avPacket);
            }
        } else {
            if (LOG_DEBUG) {
                LOGD("读取不到数据，多半是结束了!")
            }
//            playStatus->exit = true;
            av_packet_free(&avPacket);
            av_free(avPacket);

            playStatus->exit = true;
            break;

//            while (playStatus != NULL && !playStatus->exit) {
//                if (mVideo->queue->getQueueSize() > 0) {
//                    continue;
//                } else {
//                    LOGD("资源跑完了，可以释放了!!")
//                    playStatus->exit = true;
//                    break;
//                }
//
//
//            }
        }
    }

    if (LOG_DEBUG) {
        LOGD("播放完成!")
    }

    if(actionStop){
        LOGD("actionStop!!!!")
        actionStop = false;
        releaseResources();
        callJava->onCallPlayStop(CHILD_THREAD);
    }else{
        LOGD("actionComplete!@@@")
        releaseResources();
        callJava->onCallPlayCompleted(CHILD_THREAD);
    }

}


void FFmpegControl::seek(int64_t second) {
    if (duration < 0) {
        if (LOG_DEBUG) {
            LOGE("duration LessThan 0!")
        }
        return;
    }

    LOGD("seek - init : %ld ,duration: %d ", second, duration)

    //校验seek位置是否合理
    if (second >= 0 && second <= duration) {
        pthread_mutex_lock(&seek_mutex);
        playStatus->seek = true;


        //调用ffmpegSeek到执行位置
        int64_t rel = second * AV_TIME_BASE;
        avformat_seek_file(pFormatCtx, -1, INT64_MIN, rel, INT64_MAX, 0);

        //清空队列,刷新解码器队列
        if (mVideo != nullptr) {
            mVideo->queue->clearAvpacket();
            mAudio->clock = 0;
            mVideo->last_tiem = 0;
            pthread_mutex_lock(&mVideo->codecMutex);
            avcodec_flush_buffers(mVideo->avCodecContext);
            pthread_mutex_unlock(&mVideo->codecMutex);
        }


        if (mAudio != nullptr) {
            mAudio->queue->clearAvpacket();
            mAudio->clock = 0;
            mAudio->last_tiem = 0;
            avcodec_flush_buffers(mAudio->avCodecContext);
        }

        pthread_mutex_unlock(&seek_mutex);
        playStatus->seek = false;
    }

}


void FFmpegControl::pause() {
    playStatus->pause = true;

    if (mAudio != nullptr) {
        mAudio->pause();
    }

}

void FFmpegControl::resume() {
    playStatus->pause = false;

    if (mAudio != nullptr) {
        mAudio->resume();
    }
}

void FFmpegControl::release() {

    if (LOG_DEBUG) {
        LOGE("release -> 开始释放资源");
    }

    //释放ffmpge相关资源
    stop();

    if (callJava != nullptr) {
        LOGE("释放CallBack资源")
        delete callJava;
        callJava = nullptr;
    }


    if (playStatus != nullptr) {
        delete playStatus;
        playStatus = nullptr;
    }

}

void FFmpegControl::speed(float speed) {
    if (mAudio != nullptr) {
        mAudio->setSpeed(speed);
    }
}


void FFmpegControl::stop() {

//    if(playStatus->exit){
//        LOGD("已经在释放了，无需再次释放！")
//        return;
//    }

    if (LOG_DEBUG) {
        LOGE("stop -> 准备释放相关资源!!")
    }

    actionStop = true;
}

void FFmpegControl::releaseResources() {

    playStatus->exit = true;

    av_usleep(200 * 1000);
    LOGE("releaseResources")
    pthread_mutex_lock(&init_mutex);
    LOGE("releaseResources - 上锁啦!")

    if (mVideo != nullptr) {
        mVideo->release();
        delete (mVideo);
        mVideo = nullptr;
    }


    if (mAudio != nullptr) {
        mAudio->release();
        delete mAudio;
        mAudio = nullptr;
    }



    if (pFormatCtx != nullptr) {
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        pFormatCtx = nullptr;
    }

    pthread_mutex_unlock(&init_mutex);
    LOGD("releaseResources - 释放锁完毕")
}


FFmpegControl::~FFmpegControl() {
}













