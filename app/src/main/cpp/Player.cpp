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
    if (callbackHelper) {
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

void *task_start(void *args) {
    auto *player = static_cast<Player *>(args);
    player->start_();
    return 0; // 必须返回，坑，错误很难找
}

void Player::prepare_() {
    // 为什么FFmpeg源码，大量使用上下文Context？
    // 答：因为FFmpeg源码是纯C的，他不像C++、Java ， 上下文的出现是为了贯彻环境，就相当于Java的this能够操作成员
    //第一步：打开媒体地址（文件路径， 直播地址rtmp）
    formatContext = avformat_alloc_context();
    AVDictionary *dictionary = 0;
    av_dict_set(&dictionary, "timeout", "5000000", 0);// 单位微妙
    /**
    * 1，AVFormatContext *
    * 2，路径
    * 3，AVInputFormat *fmt  Mac、Windows 摄像头、麦克风， 我们目前安卓用不到
    * 4，各种设置：例如：Http 连接超时， 打开rtmp的超时  AVDictionary **options
    */
    int r = avformat_open_input(&formatContext, data_source, 0, &dictionary);
    // 释放字典
    av_dict_free(&dictionary);
    if (r) {
        //jni反射回调给java方法，并提示
        if (callbackHelper) {
            callbackHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }

    // 第二步：查找媒体中的音视频流的信息
    r = avformat_find_stream_info(formatContext, 0);
    if (r < 0) {
        //jni反射回调给java方法，并提示
        if (callbackHelper) {
            callbackHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }

    //第三步：根据流信息，流的个数，用循环来找
    // 下标 0视频  1音频
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        // 第四步：获取媒体流（视频，音频）
        AVStream *stream = formatContext->streams[i];

        //第五步：
        //从上面的流中 获取 编码解码的【参数】
        //由于：后面的编码器 解码器 都需要参数（宽高 等等）
        AVCodecParameters *parameters = stream->codecpar;

        //第六步：（根据上面的【参数】）获取编解码器
        AVCodec *codec = avcodec_find_decoder(parameters->codec_id);
        if (!codec){
            if (callbackHelper){
                callbackHelper->onError(THREAD_CHILD,FFMPEG_FIND_DECODER_FAIL);
            }
        }

        //第七步：编解码器 上下文 （这个才是真正干活的）
        AVCodecContext *codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) {
            //jni反射回调给java方法，并提示
            if (callbackHelper) {
                callbackHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            return;
        }

        //第八步：他目前是一张白纸（parameters copy codecContext）
        r = avcodec_parameters_to_context(codecContext, parameters);
        if (r < 0) {
            //jni反射回调给java方法，并提示
            if (callbackHelper) {
                callbackHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            }
            return;
        }

        // 第九步：打开解码器
        r = avcodec_open2(codecContext, codec, 0);
        if (r) {
            //jni反射回调给java方法，并提示
            if (callbackHelper) {
                callbackHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }

        //第十步：从编解码器参数中，获取流的类型 codec_type  ===  音频 视频
        if (parameters->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
            audio_channel = new AudioChannel(i, codecContext);
        } else if (parameters->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO) {
            video_channel = new VideoChannel(i, codecContext);
//            video_channel->setRenderCallback(renderCallback);
        }
    }//for end

    //第十一步: 如果流中 没有音频 也没有 视频 【健壮性校验】
    if (!audio_channel && !video_channel) {
        //JNI 反射回调到Java方法，并提示
        if (callbackHelper) {
            callbackHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        }
        return;
    }

    /**
     *  第十二步：准备成功，我们的媒体文件 OK了，通知给上层
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

    // 创建子线程 把 音频和视频 压缩包 加入队列里面去
    pthread_create(&pid_start, 0, task_start, this);
}

// 把 视频 音频 的压缩包(AVPacket *) 循环获取出来 加入到队列里面去
void Player::start_() {
    while (isPlaying) {
        // AVPacket 可能是音频 也可能是视频（压缩包）
        AVPacket *packet = av_packet_alloc();
        int ret = av_read_frame(formatContext, packet);
        if (!ret) {
            // AudioChannel    队列
            // VideioChannel   队列
            // 把我们的 AVPacket* 加入队列， 音频 和 视频
            if (video_channel && video_channel->stream_index == packet->stream_index) {
                video_channel->packets.insertToQueue(packet);
            } else if (audio_channel && audio_channel->stream_index == packet->stream_index) {
                //代表是音频
            }
        } else if (ret == AVERROR_EOF) {
            // 表示读完了，要考虑释放播放完成，表示读完了 并不代表播放完毕
        } else {
            break;// av_read_frame 出现了错误，结束当前循环
        }
    }// end while
    isPlaying = 0;
    video_channel->stop();
    audio_channel->stop();
}

void Player::setRenderCallback(RenderCallback renderCallback) {
    this->renderCallback = renderCallback;
}


