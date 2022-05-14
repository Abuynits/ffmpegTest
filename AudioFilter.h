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
//The threshold for the lowpass filter. All frequencies below this value are kepy
//TODO: remove low pass and high pass filter
//TODO: why not write header to ffmpeg.
// output: use mp3/aac. As input we tail aim4a? what browser inputs
//TODO: add 3 metrics: Start cutofff ,end cutoff, ratio of noise suppression. Signal to noise ratio.
//
//percentage of wanted volume. Present to 100% = 1.00
#define VOLUME 1.00
//The threshold for the lowpass filter. All frequencies below this value are kepy
#define LOWPASS_VAL 3000
//The threshold for the highpass filter. All frequencies above this value are kepy
#define HIGHPASS_VAL 200
//1=remove silent frames, 0=keep all silent frames (same for start and end of audio), -1 = remove in the middle as well
#define STOP_PERIOD 1
//1=remove silent frames, 0=keep all silent frames (same for start and end of audio)
#define START_PERIOD 1
//CAN CHANGE/ EDIT, DONT REALLY NEED TO
//Threshold for detection of silence. has to be negative. if it is closer to 0, it is more sensitive. -30dB was recommended
#define START_THRESHOLD -30dB
//Threshold for detection of silence. has to be negative. if it is closer to 0, it is more sensitive. -30dB was recommended
#define STOP_THRESHOLD -40dB
//the amount of time that non-silence has to be detected before it stops trimming audio
#define START_DURATION 0
//The amount of silence that you want at the end of the audio. the greater the number, the more silence at the end.
#define STOP_SILENCE 0
//The amount of silence that you want at the start of the audio. the greater the number, the more silence at the start.
#define START_SILENCE 1
//HERE: STILL VARIABLE::
//The amount of time used for determining whether you have silence in audio. default =0.02, (using longer window to have more frames to determine if have noise or not)
#define WINDOW 0.9
//set how detect silence: "peak" = faster and works better with digital silence, "rms" = default
#define DETECTION rms
//used to skip over wanted silence. very important for skipping over gaps
#define STOP_DURATION 0

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

    AVFilterContext *arnndnFilterContext = nullptr;
    const AVFilter *arnndnFilter = nullptr;

    //silence detector:
    AVFilterContext *silenceRemoverFilterContext = nullptr;
    const AVFilter *silenceRemoverFilter = nullptr;

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

    int initArnndnFilter();

    int initFormatFilter();

    int initSilenceRemoverFilter();

    int generalFilterInit(AVFilterContext **af, const AVFilter **f, const char *name) const;

    static int initByDict(AVFilterContext *afc, const char *key, const char *val);

    void initByFunctions(AVFilterContext *afc) const;
};


#endif //FFMPEGTEST5_AUDIOFILTER_H