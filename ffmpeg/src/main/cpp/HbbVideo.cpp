//
// Created by DELL on 2023/4/27.
//

#include "HbbVideo.h"

HbbVideo::HbbVideo(Playstatus *playstatus, HbbCallJava *callJava) : playstatus(playstatus),
                                                                    callJava(callJava) {
    queue = new HbbQueue(playstatus);
    pthread_mutex_init(&codecMutex, NULL);
}

void *playThread(void *data) {
    HbbVideo *hbbVideo = (HbbVideo *) data;
    hbbVideo->decode();
    pthread_exit(&hbbVideo->decodeThread);
}

void HbbVideo::play() {
    //开线程，读取数据解码
    pthread_create(decodeThread, NULL, playThread, this);
}


void HbbVideo::decode() {
    while (playstatus != nullptr && !playstatus->exit) {
        if (playstatus->seek) {
            av_usleep(1000 * 100);
            continue;
        }

        if (queue->getQueueSize() == 0) {
            // 网络不佳  请慢慢等待  回调应用层
            if (!playstatus->load) {
                playstatus->load = true;
//                callJava->onCallLoad(CHILD_THREAD, true);
                av_usleep(1000 * 100);
                continue;
            }
        }

        //从队列里读取编码数据
        AVPacket *avPacket = av_packet_alloc();
        if (queue->getAvpacket(avPacket) != 0) {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            continue;
        }

        //开始解码
        pthread_mutex_lock(&codecMutex);

        if (avcodec_send_packet(avCodecContext, avPacket) != 0) {
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            pthread_mutex_unlock(&codecMutex);
            continue;
        }

        AVFrame *avFrame = av_frame_alloc();

        if (avcodec_receive_frame(avCodecContext,avFrame) != 0) {
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            pthread_mutex_unlock(&codecMutex);
        }

        double currentSeconds = avFrame->pts * av_q2d(time_base);
        LOGE("当前播放时间@： %lf", (currentSeconds));

        if(avFrame->format != AV_PIX_FMT_YUV420P){
            //无需转换，直接传到上层播放
            callJava->onCallRenderYUV(
                    avCodecContext->width,
                    avCodecContext->height,
                    avFrame->data[0],
                    avFrame->data[1],
                    avFrame->data[2]);
            LOGE("当前视频是YUV420P格式");
        }


        pthread_mutex_unlock(&codecMutex);
    }

}


HbbVideo::~HbbVideo() {

}


