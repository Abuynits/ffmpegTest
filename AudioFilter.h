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
    AVFilter *aFilter;

    //vad filter
    AVFilterContext *vadFilterContext;
    AVFilter *vadFilter;

    //lp=low pass
    AVFilterContext *lpFilterContext;
    AVFilter *lpFilter;

    //hp = high pass
    AVFilterContext  *hpFilterContext;
    AVFilter *hpFilter;



    AudioFilter();

    int initializeAllObjets();

private:
};


#endif //FFMPEGTEST5_AUDIOFILTER_H
