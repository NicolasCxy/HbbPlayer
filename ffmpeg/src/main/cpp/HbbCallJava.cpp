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

    jmid_renderyuv = env->GetMethodID(jlz, "onCallRenderYUV", "(II[B[B[B)V");

}

HbbCallJava::~HbbCallJava() {
    jniEnv->DeleteGlobalRef(jobj);
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
    if (javaVM->AttachCurrentThread(&jniChildEnv, 0) != JNI_OK) {
        if (LOG_DEBUG) {
            LOGE("call onCallComplete worng");
        }
        return;
    }


    jbyteArray y = jniChildEnv->NewByteArray(width * height);
    jniChildEnv->SetByteArrayRegion(y, 0, width * height, reinterpret_cast<const jbyte *>(fy));

    jbyteArray u = jniChildEnv->NewByteArray(width * height / 4);
    jniChildEnv->SetByteArrayRegion(u, 0, width * height / 4, reinterpret_cast<const jbyte *>(fu));

    jbyteArray v = jniChildEnv->NewByteArray(width * height / 4);
    jniChildEnv->SetByteArrayRegion(v, 0, width * height / 4, reinterpret_cast<const jbyte *>(fv));

    jniChildEnv->CallVoidMethod(jobj, jmid_renderyuv, width, height, y, u, v);

    jniChildEnv->DeleteLocalRef(y);
    jniChildEnv->DeleteLocalRef(u);
    jniChildEnv->DeleteLocalRef(v);
    javaVM->DetachCurrentThread();
}
