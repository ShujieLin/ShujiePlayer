#include "VideoChannel.h"
VideoChannel::VideoChannel(int stream_index, AVCodecContext *codecContext)
        : BaseChannel(stream_index, codecContext) {

}

void VideoChannel::start() {
    isPlaying = 1;
}
