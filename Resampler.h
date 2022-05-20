//
// Created by Alexiy Buynitsky on 5/20/22.
//

#ifndef FFMPEGTEST5_RESAMPLER_H
#define FFMPEGTEST5_RESAMPLER_H

#include "AudioDecoder.h"

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
};

class Resampler {
public:
    Resampler(AudioDecoder *ad) {
        this->ad = ad;

    }
   int initObjects();

private:
    struct SwrContext *resampleCtx;
    AudioDecoder *ad;
};


#endif //FFMPEGTEST5_RESAMPLER_H
