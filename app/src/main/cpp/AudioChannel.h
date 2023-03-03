#ifndef AUTIO_CHANEL_H
#define AUTIO_CHANEL_H

#include "BaseChannel.h"
extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
};

class AudioChannel : public BaseChannel
{
public:
    AudioChannel(int streamIndex, AVCodecContext *codecContext);

    void stop();
};

#endif