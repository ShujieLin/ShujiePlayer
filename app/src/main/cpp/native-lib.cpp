#include <jni.h>
#include <string>
#include "Player.h"
#include "log4c.h"
#include "JNICallbackHelper.h"
#include <android/native_window_jni.h> // ANativeWindow 用来渲染画面的 == Surface对象

extern "C" {
#include <libavutil/avutil.h>
}

JavaVM *vm = 0;
Player *player = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 静态初始化 锁
ANativeWindow *window = 0;

jint JNI_OnLoad(JavaVM *vm, void *args) {
    ::vm = vm;
    return JNI_VERSION_1_6;
}

/**
 * 函数指针 实现  渲染工作
 * @param src_data
 * @param width
 * @param height
 * @param src_lineSize
 */
void renderFrame(uint8_t *src_data, int width, int height, int src_lineSize) {
    pthread_mutex_lock(&mutex);
    if (!window) {
        pthread_mutex_unlock(&mutex); // 出现了问题后，必须考虑到，释放锁，怕出现死锁问题
    }
    // 设置窗口的大小，各个属性
    // WINDOW_FORMAT_RGBA_8888最常用：32字节
    ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGBA_8888);

    ANativeWindow_Buffer windowBuffer;

    //健壮性需要
    // 如果我在渲染的时候，是被锁住的，那我就无法渲染，我需要释放 ，防止出现死锁
    if (ANativeWindow_lock(window, &windowBuffer, 0)) {
        ANativeWindow_release(window);
        window = 0;

        pthread_mutex_unlock(&mutex);
        return;
    }

    //开始真正渲染，因为window没有被锁住了，就可以把 rgba数据 ---> 字节对齐 渲染
    //填充window_buffer  画面就出来了
    uint8_t *dst_data = static_cast<uint8_t *>(windowBuffer.bits);
    int dst_linesize = windowBuffer.stride * 4;


    for (int i = 0; i < windowBuffer.height; ++i) {    // 图：一行一行显示 [高度不用管，用循环了，遍历高度]
        // 视频分辨率：426 * 240
        // 视频分辨率：宽 426
        // 426 * 4(rgba8888) = 1704
        // memcpy(dst_data + i * 1704, src_data + i * 1704, 1704); // 花屏
        // 花屏原因：1704 无法 64字节对齐，所以花屏

        // ANativeWindow_Buffer 64字节对齐的算法，  1704无法以64位字节对齐
        // memcpy(dst_data + i * 1792, src_data + i * 1704, 1792); // OK的
        // memcpy(dst_data + i * 1793, src_data + i * 1704, 1793); // 部分花屏，无法64字节对齐
        // memcpy(dst_data + i * 1728, src_data + i * 1704, 1728); // 花屏

        // ANativeWindow_Buffer 64字节对齐的算法  1728
        // 占位 占位 占位 占位 占位 占位 占位 占位
        // 数据 数据 数据 数据 数据 数据 数据 空值

        // ANativeWindow_Buffer 64字节对齐的算法  1792  空间换时间
        // 占位 占位 占位 占位 占位 占位 占位 占位 占位
        // 数据 数据 数据 数据 数据 数据 数据 空值 空值

        // FFmpeg为什么认为  1704 没有问题 ？
        // FFmpeg是默认采用8字节对齐的，他就认为没有问题， 但是ANativeWindow_Buffer他是64字节对齐的，就有问题

        // 通用的
        memcpy(dst_data + i * dst_linesize, src_data + i * src_lineSize, dst_linesize);
    }
    // 数据刷新
    ANativeWindow_unlockAndPost(window);// 解锁后 并且刷新 window_buffer的数据显示画面

    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_shujie_shujieplayer_player_Player_prepareNative(JNIEnv *env, jobject thiz,
                                                         jstring data_source) {
    const char *data_source_ = env->GetStringUTFChars(data_source, 0);
    //可能由c++子线程或者主线程进行回调
    auto *callbackHelper = new JNICallbackHelper(vm, env, thiz);
    player = new Player(data_source_, callbackHelper);
    player->setRenderCallback(renderFrame);
    player->prepare();
    env->ReleaseStringUTFChars(data_source, data_source_);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_shujie_shujieplayer_player_Player_startNative(JNIEnv *env, jobject thiz) {
    if (player) {
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

/**
 * 实例化window
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_shujie_shujieplayer_player_Player_setSurfaceNative(JNIEnv *env, jobject thiz,
                                                            jobject surface) {
    pthread_mutex_lock(&mutex);
    // 先释放之前的显示窗口
    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }
    // 创建新的窗口用于视频显示
    window = ANativeWindow_fromSurface(env, surface);
    pthread_mutex_unlock(&mutex);
}