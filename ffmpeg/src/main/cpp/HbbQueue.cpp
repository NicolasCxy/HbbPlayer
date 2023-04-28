//
// Created by DELL on 2023/4/27.
//

#include "HbbQueue.h"

HbbQueue::HbbQueue(Playstatus *playstatus) {
    this->playstatus = playstatus;
    pthread_mutex_init(&mutexPacket, NULL);
    pthread_cond_init(&condPacket, NULL);
}

HbbQueue::~HbbQueue() {

}

int HbbQueue::putAvpacket(AVPacket *packet) {
    pthread_mutex_lock(&mutexPacket);

    queuePacket.push(packet);
    pthread_cond_signal(&condPacket);
    pthread_mutex_unlock(&mutexPacket);
    return 0;
}

int HbbQueue::getAvpacket(AVPacket *packet) {
    pthread_mutex_lock(&mutexPacket);

    while (playstatus != NULL && !playstatus->exit) {
        if (queuePacket.size() > 0) {
            AVPacket *avPacket = queuePacket.front();
            if (av_packet_ref(packet, avPacket) == 0) {
                queuePacket.pop();
            }
            av_packet_free(&avPacket);
            av_free(avPacket);
            avPacket = NULL;
            break;
        } else {
            pthread_cond_wait(&condPacket, &mutexPacket);
        }
    }
    pthread_mutex_unlock(&mutexPacket);
    return 0;
}

int HbbQueue::getQueueSize() {
    int size = 0;
    pthread_mutex_lock(&mutexPacket);
    size = queuePacket.size();
    pthread_mutex_unlock(&mutexPacket);
    return size;
}

void HbbQueue::clearAvpacket() {
    pthread_cond_signal(&condPacket);
    pthread_mutex_unlock(&mutexPacket);

    while (!queuePacket.empty()) {
        //取出队列第一个元素
        AVPacket *packet1 = queuePacket.front();
        queuePacket.pop();
        av_packet_free(&packet1);
        av_free(packet1);
        packet1 = NULL;
    }
    pthread_mutex_unlock(&mutexPacket);
}
