#include "VideoChannel.h"

VideoChannel::VideoChannel(int stream_index, AVCodecContext *codecContext) : BaseChannel(
        stream_index, codecContext) {

}

VideoChannel::~VideoChannel() {


}


void *task_video_decode(void *args) {
    auto *video_channel = static_cast<VideoChannel *>(args);
    video_channel->video_decode();
    return 0;
}

void *task_video_play(void *args) {
    auto *video_channel = static_cast<VideoChannel *> (args);
    video_channel->video_play();
    return 0;
}

// 视频：
// 1.把队列里面的压缩包(AVPacket *)取出来，然后解码成（AVFrame * ）原始包 ----> 保存队列
// 2.把队列里面的原始包(AVFrame *)取出来， 播放
void VideoChannel::start() {
    packets.setWork(1);
    frames.setWork(1);
    //第一个线程： 视频：取出队列的压缩包 进行解码 解码后的原始包 再push队列中去
    pthread_create(&pid_video_decode, 0, task_video_decode, this);
    // 第二线线程：视频：从队列取出原始包，播放
    pthread_create(&pid_video_play, 0, task_video_play, this);
}


void VideoChannel::stop() {

}

void VideoChannel::video_decode() {
    AVPacket *pkt = 0;
    while (isPlaying) {
        int ret = packets.getQueueAndDel(pkt);
        if (!isPlaying) {
            break;
        }

        if (!ret) {//假如没有拿到数据，重复循环，直到拿到数据
            continue;
        }
        // 最新的FFmpeg，和旧版本差别很大， 新版本：1.发送pkt（压缩包）给缓冲区，  2.从缓冲区拿出来（原始包）
        ret = avcodec_send_packet(codecContext, pkt);
        releaseAVPacket(&pkt); // FFmpeg源码缓存一份pkt，大胆释放即可

        if (ret != 0) {//错误
            break;
        }

        AVFrame *avFrame = av_frame_alloc();//
        ret = avcodec_receive_frame(codecContext, avFrame);
        if (ret == AVERROR(EAGAIN)) {
            // B帧  B帧参考前面成功  B帧参考后面失败   可能是P帧没有出来，再拿一次就行了
            continue;
        } else if (ret != 0) {
            break;
        }

        frames.insertToQueue(avFrame);
    }//end while

    releaseAVPacket(&pkt);
}

void VideoChannel::video_play() {
    AVFrame *avFrame = 0;
    uint8_t *dst_data[4];// RGBA
    int dst_linesize[4];

    // 原始包（YUV数据）  ---->[libswscale]   Android屏幕（RGBA数据）
    //给 dst_data 申请内存   width * height * 4 xxxx
    av_image_alloc(dst_data, dst_linesize, codecContext->width, codecContext->height,
                   AV_PIX_FMT_RGBA, 1);

    SwsContext *swsContext = sws_getContext(
            //输入：
            codecContext->width,
            codecContext->height,
            codecContext->pix_fmt,// 自动获取 xxx.mp4 的像素格式  AV_PIX_FMT_YUV420P // 写死的
            //输出：
            codecContext->width,
            codecContext->height,
            AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);

    while (!isPlaying) {
        int ret = frames.getQueueAndDel(avFrame);
        if (!isPlaying) {
            break;
        }

        if (ret != 1) {// 哪怕是没有成功，也要继续（假设：你生产太慢(原始包加入队列)，我消费就等一下你）
            continue;
        }

        // 格式转换 yuv ---> rgba
        sws_scale(swsContext, avFrame->data, avFrame->linesize, 0, codecContext->height, dst_data,
                  dst_linesize);
        renderCallback(dst_data[0], codecContext->width, codecContext->height, dst_linesize[0]);
        releaseAVFrame(&avFrame);// 释放原始包，因为已经被渲染完了
    }

    releaseAVFrame(&avFrame); // 出现错误，所退出的循环，都要释放frame
    isPlaying = 0;
    av_free(dst_data);
    sws_freeContext(swsContext);
}

void VideoChannel::setRenderCallback(RenderCallback renderCallback) {
    this->renderCallback = renderCallback;
}
