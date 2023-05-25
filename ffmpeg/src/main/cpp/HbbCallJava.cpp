//
// Created by DELL on 2023/4/27.
//

#include "HbbCallJava.h"

HbbCallJava::HbbCallJava(_JavaVM *javaVM, JNIEnv *env, jobject *obj) : javaVM(javaVM), jniEnv(env) {

    this->javaVM = javaVM;
    this->jniEnv = env;
    this->jobj = *obj;
    this->jobj = env->NewGlobalRef(jobj);

    jclass jlz = jniEnv->GetObjectClass(jobj);
    if (!jlz) {
        if (LOG_DEBUG) {
            LOGE("get jclass wrong!");
        }
        return;
    }

    jmid_parpared = jniEnv->GetMethodID(jlz, "onBeReady", "()V");

    jmid_renderyuv = jniEnv->GetMethodID(jlz, "onCallRenderYUV", "(II[B[B[B)V");

    jmid_timeinfo = jniEnv->GetMethodID(jlz, "onCallTimeInfo", "(II)V");

    jmid_playcompleted = jniEnv->GetMethodID(jlz,"onPlaybackCompleted","()V");

    jmid_playstop = jniEnv->GetMethodID(jlz,"onPlaybackStop","()V");

}

HbbCallJava::~HbbCallJava() {
    jniEnv->DeleteGlobalRef(jobj);
//    delete javaVM;
    this->javaVM = nullptr;
    this->jniEnv = nullptr;
    this->jobj = nullptr;
}

void HbbCallJava::onCallParpared(int type) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_parpared);
    } else if (type == CHILD_THREAD) {
        if (javaVM->AttachCurrentThread(&jniChildEnv, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv worng");
            }
            return;
        }
        jniChildEnv->CallVoidMethod(jobj, jmid_parpared);
        javaVM->DetachCurrentThread();
    }
}

void HbbCallJava::onCallRenderYUV(int width, int height, uint8_t *fy, uint8_t *fu, uint8_t *fv) {
    JNIEnv *jniEnv;
    if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
        if (LOG_DEBUG) {
            LOGE("call onCallComplete worng");
        }
        return;
    }

    jbyteArray y = jniEnv->NewByteArray(width * height);
    jniEnv->SetByteArrayRegion(y, 0, width * height, reinterpret_cast<const jbyte *>(fy));

    jbyteArray u = jniEnv->NewByteArray(width * height / 4);
    jniEnv->SetByteArrayRegion(u, 0, width * height / 4, reinterpret_cast<const jbyte *>(fu));

    jbyteArray v = jniEnv->NewByteArray(width * height / 4);
    jniEnv->SetByteArrayRegion(v, 0, width * height / 4, reinterpret_cast<const jbyte *>(fv));

    jniEnv->CallVoidMethod(jobj, jmid_renderyuv, width, height, y, u, v);

    jniEnv->DeleteLocalRef(y);
    jniEnv->DeleteLocalRef(u);
    jniEnv->DeleteLocalRef(v);
    javaVM->DetachCurrentThread();
}

void HbbCallJava::onCallTimeInfo(int type, int curr, int total) {

    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_timeinfo, curr, total);
    } else {
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("call onCallTimeInfo worng");
            }
            return;
        }
        jniEnv->CallVoidMethod(jobj, jmid_timeinfo, curr, total);
        javaVM->DetachCurrentThread();
    }
}

void HbbCallJava::onCallPlayCompleted(int type) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_playcompleted);
    } else {
        if (javaVM->AttachCurrentThread(&jniChildEnv, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv worng CallTime");
            }
            return;
        }
        jniChildEnv->CallVoidMethod(jobj, jmid_playcompleted);
//        javaVM->DetachCurrentThread();
    }
}

void HbbCallJava::onCallPlayStop(int type) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_playcompleted);
    } else {
        if (javaVM->AttachCurrentThread(&jniChildEnv, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv worng CallTime");
            }
            return;
        }
        jniChildEnv->CallVoidMethod(jobj, jmid_playstop);
//        javaVM->DetachCurrentThread();
    }
}
