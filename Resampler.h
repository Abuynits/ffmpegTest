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

<<<<<<< HEAD
    int numSrcChannels, dstNumChannels;
    struct SwrContext *resampleCtx;
    AudioDecoder *ad;

    uint8_t **srcData = nullptr, **dstData = nullptr;
    int srcLineSize, dstLineSize;
    int srcNumChannels = 0;
    int dstBufferSize = 0;

    //TODO: find the number of src samples
    int srcNumSamples = 4096, dstNumSamples = 0, maxDstNumSamples;
=======
    int numSrcChannels, numDstChannels;
    struct SwrContext *resampleCtx;
    AudioDecoder *ad;
    int dstBufferSize;
    uint8_t **srcData = nullptr, **dstData = nullptr;
    int srcLineSize, dstLineSize;

    int numSrcSamples = 0, numDstSamples = 0, maxDstNumSamples;
>>>>>>> ea6eb83e5879537ab8d025fbb8bf0e3b79bd9d61
private:


};


#endif //FFMPEGTEST5_RESAMPLER_H
