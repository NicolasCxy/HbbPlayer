//
// Created by DELL on 2023/4/27.
//

#include <jni.h>
#include <string>
#include "AndroidLog.h"

#include "HbbCallJava.h"
#include "FFmpegControl.h"
#include "Playstatus.h"


_JavaVM *javaVM = NULL;
HbbCallJava *callJava;
Playstatus *playStatus;
FFmpegControl *ffControl;

extern "C" JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved) {
    jint result = -1;
    javaVM = vm;
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return result;
    }
    return JNI_VERSION_1_4;
}



extern "C" JNIEXPORT void JNICALL
Java_com_hbb_ffmpeg_palyer_HbbPlayer_n_1prepare(JNIEnv *env, jobject thiz, jstring path_) {
    /**
     * 1、创建FFmpgeControl，将url传递进去
     * 2、内部开线程调用FFmpge api读取加载数据
     * 3、创建videoModel和AudioModel，获取基本信息
     * 4、回调到上层提给出准备成功回调
     */

    const char *path = env->GetStringUTFChars(path_, 0);

    if(ffControl == nullptr){
        callJava = new HbbCallJava(javaVM, env, &thiz);
        playStatus = new Playstatus();
        ffControl = new FFmpegControl(callJava, playStatus, path);
    }

    ffControl->prepare();
//    env->ReleaseStringUTFChars(path_,path);

}

extern "C" JNIEXPORT void JNICALL
Java_com_hbb_ffmpeg_palyer_HbbPlayer_n_1start(JNIEnv *env, jobject thiz) {
    /**
     * 1、读取数据到队列中
     * 2、video、audio模块开循环读取数据
     * 3、解码渲染
     */

    if(ffControl != nullptr){
        ffControl->start();
    }
}