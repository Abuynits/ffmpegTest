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
    AudioDecoder *ad = nullptr;
    AVFilterGraph *filterGraph = nullptr;
    //used to pass in the initial frame (the first 'filter' in the graph)
    AVFilterContext *srcFilterContext = nullptr;
    const AVFilter *srcFilter = nullptr;


    AVFilterContext *lpFilterContext = nullptr;
    const AVFilter *lpFilter = nullptr;

    AVFilterContext *hpFilterContext = nullptr;
    const AVFilter *hpFilter = nullptr;
    //vad filter
    AVFilterContext *volumeFilterContext = nullptr;
    const AVFilter *volumeFilter = nullptr;

    //the end point of the filter - this is how you specify what to write
    AVFilterContext *sinkFilterContext = nullptr;
    const AVFilter *sinkFilter = nullptr;

    AVFilterContext *aFormatContext = nullptr;
    const AVFilter *aFormatFilter = nullptr;
/*
 * want to filter out everything between 500hz or 200hz and 3000hz
 */

    AudioFilter(AudioDecoder *ad);

    int initializeAllObjets();

    void closeAllObjects();

private:
    int initSrcFilter();

    int initSinkFilter();

    int initVolumeFilter();

    int initLpFilter();

    int initHpFilter();

    int initFormatFilter();

    static int initByDict(AVFilterContext *afc, const char *key, const char *val);

    void initByFunctions(AVFilterContext *afc);
};


#endif //FFMPEGTEST5_AUDIOFILTER_H
