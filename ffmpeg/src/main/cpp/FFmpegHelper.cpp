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

    if (ffControl == nullptr) {
        callJava = new HbbCallJava(javaVM, env, &thiz);
        playStatus = new Playstatus();
        ffControl = new FFmpegControl(callJava, playStatus);
    }

    ffControl->setUrl(path);
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

    if (ffControl != nullptr) {
        LOGD("ffControl Start!");
        ffControl->start();
    } else {
        LOGE("ffControl is Null!!");
    }
}
extern "C" JNIEXPORT void JNICALL
Java_com_hbb_ffmpeg_palyer_HbbPlayer_n_1seek(JNIEnv *env, jobject thiz, jint second) {
    /**
     * 1、校验长度、判断seek值合不合理
     * 2、暂停数据读取和解码，调用av_seek 进行seek 到指定位置
     * 3、清空队列，刷新编解码缓冲区
     */
    if (ffControl != nullptr) {
        ffControl->seek(second);
    }


}
extern "C" JNIEXPORT void JNICALL
Java_com_hbb_ffmpeg_palyer_HbbPlayer_n_1stop(JNIEnv *env, jobject thiz) {
    //释放资源
    if (ffControl != nullptr) {
        ffControl->stop();
        delete ffControl;
        ffControl = nullptr;
    }

}

extern "C" JNIEXPORT void JNICALL
Java_com_hbb_ffmpeg_palyer_HbbPlayer_n_1pause(JNIEnv *env, jobject thiz) {
    //暂停
    if (ffControl != nullptr) {
        ffControl->pause();
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_hbb_ffmpeg_palyer_HbbPlayer_n_1resume(JNIEnv *env, jobject thiz) {
    //恢复播放
    if (ffControl != nullptr) {
        ffControl->resume();
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_hbb_ffmpeg_palyer_HbbPlayer_n_1speed(JNIEnv *env, jobject thiz, jfloat speed) {
    //倍速
    if (ffControl != nullptr) {
        ffControl->speed(speed);
    }
}