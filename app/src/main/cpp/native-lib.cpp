#include <jni.h>
#include <string>
#include "Player.h"
//#include "JNICallbackHelper.h"

extern "C" {
#include <libavutil/avutil.h>
}

JavaVM *vm = 0;
Player *player = 0;

jint JNI_OnLoad(JavaVM *vm, void *args) {
    ::vm = vm;
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_shujie_shujieplayer_player_Player_prepareNative(JNIEnv *env, jobject thiz,
                                                         jstring data_source) {
    const char *data_source_ = env->GetStringUTFChars(data_source, 0);
    //可能由c++子线程或者主线程进行回调
    auto *callbackHelper = new JNICallbackHelper(vm, env, thiz);
    player = new Player(data_source_, callbackHelper);
    player->prepare();
    env->ReleaseStringUTFChars(data_source, data_source_);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_shujie_shujieplayer_player_Player_startNative(JNIEnv *env, jobject thiz) {
    if (player){
        player->start();
    }
}


extern "C"
JNIEXPORT void JNICALL
Java_com_shujie_shujieplayer_player_Player_stopNative(JNIEnv *env, jobject thiz) {
    // TODO: implement stopNative()
}
extern "C"
JNIEXPORT void JNICALL
Java_com_shujie_shujieplayer_player_Player_releaseNative(JNIEnv *env, jobject thiz) {
    // TODO: implement releaseNative()
}