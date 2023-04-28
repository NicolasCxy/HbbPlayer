//
// Created by DELL on 2023/4/27.
//

#ifndef HBBPLAYER_HBBCALLJAVA_H
#define HBBPLAYER_HBBCALLJAVA_H

#include "jni.h"
#include <linux/stddef.h>
#include "AndroidLog.h"

#define MAIN_THREAD 0
#define CHILD_THREAD 1


class HbbCallJava {
public:
    _JavaVM *javaVM = NULL;
    JNIEnv *jniEnv = NULL;
    JNIEnv *jniChildEnv = NULL; //子线程Env
    jobject jobj;

    jmethodID jmid_parpared;
    jmethodID jmid_timeinfo;
    jmethodID jmid_renderyuv;
public:
    HbbCallJava(_JavaVM *javaVM, JNIEnv *env, jobject *obj);

    ~HbbCallJava();

    void onCallParpared(int type);

    void onCallTimeInfo(int type, int curr, int total);

    void onCallRenderYUV(int width, int height, uint8_t *fy, uint8_t *fu, uint8_t *fv);
};


#endif //HBBPLAYER_HBBCALLJAVA_H
