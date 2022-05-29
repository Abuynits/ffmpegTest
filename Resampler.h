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
#include <stdio.h>

#include "libavformat/avformat.h"
#include "libavformat/avio.h"

#include "libavcodec/avcodec.h"

#include "libavutil/audio_fifo.h"
#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/channel_layout.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"
};

class Resampler {
public:
    Resampler(AudioDecoder *ad) {
        this->ad = ad;

    }

    int initObjects();


    int numSrcChannels, dstNumChannels;
    struct SwrContext *resampleCtx;
    AudioDecoder *ad;

    uint8_t **srcData = nullptr, **dstData = nullptr;
    int srcLineSize, dstLineSize;
    int srcNumChannels = 0;
    int dstBufferSize = 0;

    //TODO: find the number of src samples
    int srcNumSamples = 4096, dstNumSamples = 0, maxDstNumSamples;

private:


};


#endif //FFMPEGTEST5_RESAMPLER_H
