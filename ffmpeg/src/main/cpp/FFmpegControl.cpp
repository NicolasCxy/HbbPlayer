//
// Created by DELL on 2023/4/27.
//

#include "FFmpegControl.h"


FFmpegControl::FFmpegControl(HbbCallJava *callJava, Playstatus *playStatus, const char *url) {
    this->callJava = callJava;
    this->playStatus = playStatus;
    this->url = url;

    LOGD("ffmpegInit -> 构造Path :%s", url);

    pthread_mutex_init(&seek_mutex, NULL);
    pthread_mutex_init(&init_mutex, NULL);
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
    LOGD("ffmpegInit -> ptah :%s", url);
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
                mVideo->defaultDelayTime = 1.0 / fps;
            }
        }
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

    if (mVideo == NULL) {
        if (LOG_DEBUG) {
            LOGE("video is null");
            return;
        }
    }

    mVideo->play();

    while (playStatus != nullptr && playStatus->exit) {
        if (playStatus->seek) {
            continue;
        }

        if (playStatus->pause) {
            continue;
        }

        AVPacket *avPacket = av_packet_alloc();
        if (av_read_frame(pFormatCtx, avPacket) == 0) {
            if(avPacket->stream_index == mVideo->streamIndex){      //视频模块
                mVideo->queue->putAvpacket(avPacket);
            }else{
                av_packet_free(&avPacket);
                av_free(avPacket);
            }
        }else{

            if (LOG_DEBUG) {
                LOGD("读取不到数据，多半是结束了");
            }
            playStatus->exit = true;
            av_packet_free(&avPacket);
            av_free(avPacket);
        }
    }


    if (LOG_DEBUG) {
        LOGD("播放完成");
    }

}

FFmpegControl::~FFmpegControl() {
}



