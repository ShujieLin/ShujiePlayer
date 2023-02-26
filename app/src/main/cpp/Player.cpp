#include "Player.h"

Player::Player(const char *string, JNICallbackHelper *pHelper) {
    //如果被释放会造成悬空指针
    /*this->data_source = data_source;*/
    //需要添加一个长度给 "\0"
    this->data_source = new char[strlen(string) + 1];//深拷贝
    stpcpy(this->data_source, string);

    this->callbackHelper = pHelper;
}

Player::~Player() {
    if (data_source) {
        delete data_source;
        data_source = 0;
    }
    if (callbackHelper){
        delete callbackHelper;
        callbackHelper = 0;
    }
}

void *task_prepare(void *args) {
//    avformat_open_input(0,thi)
    auto *player = static_cast<Player *>(args);
    player->prepare_();
    return 0;//必须返回
}

void Player::prepare_() {
    formatContext = avformat_alloc_context();
    AVDictionary  * dictionary = 0;
    av_dict_set(&dictionary,"timeout","5000000",0);

    /**
     * 1
     * 2.路径
     * 3.mac windows 摄像头 麦克风
     *
     */
    int r = avformat_open_input(&formatContext,data_source,0,&dictionary);
    //释放
    av_dict_free(&dictionary);
    if (r){
        //jni反射回调给java方法，并提示
        if (callbackHelper){
            callbackHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }

    r = avformat_find_stream_info(formatContext,0);
    if (r < 0){
        //jni反射回调给java方法，并提示
        if (callbackHelper){
            callbackHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }

    //获取流的数量
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        //获取媒体流，视频/音频
        AVStream  *stream = formatContext->streams[i];
        //获取编码的参数
        AVCodecParameters *parameters = stream->codecpar;

        //获取编码解码器
        AVCodec *codec = avcodec_find_decoder(parameters->codec_id);

        //编解码器 上下文
        AVCodecContext *codecContext = avcodec_alloc_context3(codec);
        if (!codecContext){
            //jni反射回调给java方法，并提示
            if (callbackHelper){
                callbackHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            return;
        }

        //赋值给上下文
        r = avcodec_parameters_to_context(codecContext,parameters);
        if (r<0){
            //jni反射回调给java方法，并提示
            if (callbackHelper){
                callbackHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            }
            return;
        }

        //打开解码器
        r = avcodec_open2(codecContext,codec,0);
        if (r){
            //jni反射回调给java方法，并提示
            if (callbackHelper){
                callbackHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }

        //从编解码器中获取流的类型
        if (parameters->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO){
            audio_channel = new AudioChannel();
        } else if (parameters->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO){
            video_channel = new VideoChannel();
        }
    }//for end

    //假如没有音频和视频流
    if (!audio_channel && !video_channel) {
        //JNI 反射回调到Java方法，并提示
        if (callbackHelper){
            callbackHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }

    /**
     * 准备成功，我们的媒体文件 OK了，通知给上层
     */
    if (callbackHelper) {
        callbackHelper->onPrepared(THREAD_CHILD);
    }

}


void Player::prepare() {
    //创建子线程
    pthread_create(&pid_prepare, 0, task_prepare, this);
}

void Player::start() {
    isPlaying = 1;
    // 视频：1.解码    2.播放
    // 1.把队列里面的压缩包(AVPacket *)取出来，然后解码成（AVFrame * ）原始包 ----> 保存队列
    // 2.把队列里面的原始包(AVFrame *)取出来， 播放
    if (video_channel) {
        video_channel->start();
    }
}

void Player::start_() {

}

