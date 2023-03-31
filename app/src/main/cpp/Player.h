#ifndef SHUJIEPLAYER_PLAYER_H
#define SHUJIEPLAYER_PLAYER_H


#include <cstring>
#include <pthread.h>
#include "AudioChannel.h"
#include "VideoChannel.h"
#include "JNICallbackHelper.h"
#include "util.h"

extern "C"{
#include <libavformat/avformat.h>
}
class Player{

private:
    char *data_source = 0;
    pthread_t pid_prepare;
    pthread_t pid_start;
    AVFormatContext *formatContext = 0;
    AudioChannel *audio_channel = 0;
    VideoChannel *video_channel = 0;
    JNICallbackHelper *callbackHelper = 0;
    bool isPlaying; // 是否播放
    RenderCallback renderCallback;
public:
    Player(const char *string, JNICallbackHelper *pHelper);

    virtual ~Player();

    void prepare();
    void prepare_();

    void start();
    void start_();

    void setRenderCallback(RenderCallback renderCallback);

};
#endif