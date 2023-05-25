//
// Created by DELL on 2023/4/27.
//

#include "HbbVideo.h"

HbbVideo::HbbVideo(Playstatus *playstatus, HbbCallJava *callJava) : playstatus(playstatus),
                                                                    callJava(callJava) {
    queue = new HbbQueue(playstatus);
    pthread_mutex_init(&codecMutex, NULL);
}

void *playVideo(void *data) {
    HbbVideo *hbbVideo = (HbbVideo *) data;
    hbbVideo->decode();
//    pthread_exit(&hbbVideo->decodeThread);
}

void HbbVideo::play() {
    //开线程，读取数据解码
    pthread_create(&decodeThread, NULL, playVideo, this);
}


void HbbVideo::decode() {
    LOGE("开始解码")
    //记录下一帧时间
    next_frame_time = steady_clock::now() + kFrameTime;

    while (playstatus != nullptr && !playstatus->exit) {
//        LOGE("开始解码222 value: %f",defaultDelayTime)
        if (playstatus->seek) {
            av_usleep(1000 * 100);
            continue;
        }

        //暂停状态就不读数据
        if(playstatus->pause){
            av_usleep(1000 * 50);
            continue;
        }
//
        if (queue->getQueueSize() == 0) {
            // 网络不佳  请慢慢等待  回调应用层
            if (!playstatus->load) {
                playstatus->load = true;
                av_usleep(1000 * 100);
                continue;
            }
        }

        //记录下一帧处理时间
        next_frame_time += kFrameTime;

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
        if (avcodec_receive_frame(avCodecContext, avFrame) != 0) {
            av_frame_free(&avFrame);
            av_free(avFrame);
            avFrame = NULL;
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            pthread_mutex_unlock(&codecMutex);
            continue;
        }

//        av_usleep(defaultDelayTime  * AV_TIME_BASE);


        //获取pts,拿到音频播放时间做对比，求出差值
        double diff = getFrameDiffTime(avFrame);
        //根据差值计算延迟时间
        double diffValue = getDelayTime(diff);
//        LOGD("开始解码视频@@： %lf ", (diffValue) )
        av_usleep(diffValue * AV_TIME_BASE);


        //回调播放时间给到上层
//        clock = avFrame->pts * av_q2d(time_base);
////        LOGD("当前播放时间@@： %lf ", (clock) )
//        if (clock - last_tiem > 1) {
//            last_tiem = clock;
//            callJava->onCallTimeInfo(CHILD_THREAD, clock, duration);
//        }

        if (avFrame->format != AV_PIX_FMT_YUV420P) {
            //无需转换，直接传到上层播放
            callJava->onCallRenderYUV(
                    avCodecContext->width,
                    avCodecContext->height,
                    avFrame->data[0],
                    avFrame->data[1],
                    avFrame->data[2]);
            LOGE("当前视频是YUV420P格式");
        } else {
            AVFrame *pFrameYUV420P = av_frame_alloc();
            //获取对应格式数据大小
            int buffSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, avCodecContext->width,
                                                    avCodecContext->height, 1);
            //创建buffer缓存
            uint8_t *buffer = static_cast<uint8_t *>(av_malloc(buffSize));

            //进行关联
            av_image_fill_arrays(pFrameYUV420P->data, pFrameYUV420P->linesize, buffer,
                                 AV_PIX_FMT_YUV420P, avCodecContext->width,
                                 avCodecContext->height, 1);

            //创建转换器
            SwsContext *sws_ctx = sws_getContext(avCodecContext->width, avCodecContext->height,
                                                 avCodecContext->pix_fmt, avCodecContext->width,
                                                 avCodecContext->height, AV_PIX_FMT_YUV420P,
                                                 SWS_BICUBIC, NULL, NULL, NULL);

            //创建失败
            if (!sws_ctx) {
                av_frame_free(&pFrameYUV420P);
                av_free(pFrameYUV420P);
                av_free(buffer);
                pthread_mutex_unlock(&codecMutex);
                continue;
            }

            //转换缩放
            sws_scale(sws_ctx, avFrame->data, avFrame->linesize, 0, avFrame->height,
                      pFrameYUV420P->data, pFrameYUV420P->linesize);

            //反馈到上层渲染
            if (callJava != nullptr) {
                callJava->onCallRenderYUV(avCodecContext->width,
                                          avCodecContext->height,
                                          pFrameYUV420P->data[0],
                                          pFrameYUV420P->data[1],
                                          pFrameYUV420P->data[2]);
            }


            av_frame_free(&pFrameYUV420P);
            av_free(pFrameYUV420P);
            av_free(buffer);
            sws_freeContext(sws_ctx);
        }

        av_frame_free(&avFrame);
        av_free(avFrame);
        avFrame = NULL;
        av_packet_free(&avPacket);
        av_free(avPacket);
        avPacket = NULL;
        pthread_mutex_unlock(&codecMutex);

//        sleepProcess();
    }
    LOGE("退出线程!!")
    pthread_exit(&decodeThread);

}

/**
 * 计算休眠时间，并且进行休眠
 */
void HbbVideo::sleepProcess() {
    // 根据当前时间和目标时间计算需要等待的时间间隔
//    microseconds sleep_time = duration_cast<microseconds>(next_frame_time - steady_clock::now());
//
//    if (sleep_time > microseconds(0)) {
//        // 等待到达下一个目标时间点
////        sleep_for(sleep_time);
//        av_usleep(sleep_time.count());
//    } else {
//        // 下一个时间点已经过期，即超时或处理延迟
//        next_frame_time = steady_clock::now();
//    }


    //将当前播放时间与音频播放时间做对比

}

double HbbVideo::getFrameDiffTime(AVFrame *avFrame) {
    //获取PTS
    double pts = avFrame->pts;
    if (pts == AV_NOPTS_VALUE) {
        pts = 0;
    }

    pts *= av_q2d(time_base);

    if (pts > 0) {
        clock = pts;
    }

    double diff = audio->clock - clock;
    return diff;
}


double HbbVideo::getDelayTime(double diff) {

    //音频比视频快3毫秒,加快播放速度
    if (diff > 0.003) {
        delayTime = delayTime * 2 / 3;
        //做上限处理
        if (delayTime < defaultDelayTime / 2) {
            delayTime = defaultDelayTime * 2 / 3;
        } else if (delayTime > defaultDelayTime * 2) {
            delayTime = defaultDelayTime * 2;
        }
    } else if (diff < -0.003) {
        delayTime = delayTime * 3 / 2;
        if (delayTime < defaultDelayTime / 2) {
            delayTime = defaultDelayTime * 2 / 3;
        } else if (delayTime > defaultDelayTime * 2) {
            delayTime = defaultDelayTime * 2;
        }
    }

    //如果大于500ms
    if (diff > 0.5) {       //音频快，直接不延迟
        delayTime = 0;
    } else if (diff < -0.5) {   //视频快，延迟默认帧率速度 * 2
        delayTime = delayTime * 2;
    }

    //大于5秒
    if (diff > 5) {
        queue->clearAvpacket();
        delayTime = defaultDelayTime;
    } else if (diff < -5) {
        queue->clearAvpacket();
        delayTime = defaultDelayTime;
    }

    return delayTime;
}


void HbbVideo::release() {
    LOGE("释放Video资源!")

    if (queue != nullptr) {
        queue->clearAvpacket();
        delete (queue);
        queue = nullptr;
    }
//
    if (avCodecContext != nullptr) {
        avcodec_close(avCodecContext);
        avcodec_free_context(&avCodecContext);
        avCodecContext = nullptr;
    }


}

HbbVideo::~HbbVideo() {

}






