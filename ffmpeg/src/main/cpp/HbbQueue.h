//
// Created by DELL on 2023/4/27.
//

#ifndef HBBPLAYER_HBBQUEUE_H
#define HBBPLAYER_HBBQUEUE_H


#include "queue"
#include "pthread.h"
#include "AndroidLog.h"
#include "Playstatus.h"

extern "C"
{
#include "libavcodec/avcodec.h"
}


class HbbQueue {
public:
    std::queue<AVPacket *> queuePacket;
    pthread_mutex_t mutexPacket;
    pthread_cond_t condPacket;
    Playstatus *playstatus = NULL;

public:

    HbbQueue(Playstatus*playstatus);

    ~HbbQueue();

    int putAvpacket(AVPacket *packet);

    int getAvpacket(AVPacket *packet);


    int getQueueSize();

    void clearAvpacket();
};


#endif //HBBPLAYER_HBBQUEUE_H
