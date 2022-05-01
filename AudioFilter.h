//
// Created by Alexiy Buynitsky on 4/29/22.
//

#ifndef FFMPEGTEST5_AUDIOFILTER_H
#define FFMPEGTEST5_AUDIOFILTER_H

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


#include <cstdio>
#include <iostream>
#include "AudioDecoder.h"

using namespace std;
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}

class AudioFilter {
public:
    AVFilterGraph *filterGraph;
    //used to pass in the initial frame (the first 'filter' in the graph)
    AVFilterContext *srcFilterContext= nullptr;
    const AVFilter *srcFilter= nullptr;

    //vad filter
    AVFilterContext *volumeFilterContext = nullptr;
    const AVFilter *volumeFilter = nullptr;

    //the end point of the filter - this is how you specify what to write
    AVFilterContext *sinkFilterContext= nullptr;
    const AVFilter *sinkFilter = nullptr;

    AVFilterInOut *inputs = nullptr;
    AVFilterInOut *outputs = nullptr;
//hold arguements for the filter creation
    char args[512];
    //link to the AVfilter
    const AVFilterLink *outlink = nullptr;


    AudioFilter();

    int initializeAllObjets(AudioDecoder *ad, int audio_stream_index = 0);

private:
};


#endif //FFMPEGTEST5_AUDIOFILTER_H
