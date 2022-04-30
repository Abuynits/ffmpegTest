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
#include "libavutil/channel_layout.h"
#include "libavutil/md5.h"
#include "libavutil/mem.h"
#include "libavutil/opt.h"
#include "libavutil/samplefmt.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
}

class AudioFilter {
public:
    AVFilterGraph *filterGraph;
    //used to pass in the initial frame (the first 'filter' in the graph)
    AVFilterContext *aFilterContext;
    const AVFilter *aFilter;

    //vad filter
    AVFilterContext *vadFilterContext;
    const AVFilter *vadFilter;

    //lp=low pass
    AVFilterContext *lpFilterContext;
    const AVFilter *lpFilter;

    //hp = high pass
    AVFilterContext *hpFilterContext;
    const AVFilter *hpFilter;

    //the end point of the filter - this is how you specify what to write
    AVFilterContext *aSinkFilterContext;
    AVFilter *aSinkFilter;


    AudioFilter();

    int initializeAllObjets();

private:
};


#endif //FFMPEGTEST5_AUDIOFILTER_H
