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

//percentage of wanted volume. Present to 100% = 1.00
#define VOLUME 1.00
//sqrt(sum of all squares of signals)
#define LOWPASS_VAL 10000
//The threshold for the highpass filter. All frequencies above this value are kepy
#define HIGHPASS_VAL 25
//============silence remove filter===================
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
#define START_SILENCE 0.9
//HERE: STILL VARIABLE::
//The amount of time used for determining whether you have silence in audio. default =0.02, (using longer window to have more frames to determine if have noise or not)
#define WINDOW 0.9
//set how detect silence: "peak" = faster and works better with digital silence, "rms" = default
#define DETECTION rms
//used to skip over wanted silence. very important for skipping over gaps
#define STOP_DURATION 0

//============stat filter===================
//the length of the audio used to detect silence with rms: 0.01 = 1 millisecond
#define AUDIO_SAMPLE_LENGTH 0.01
//the number of frames used to process the stats - 1 = reset after first frames
#define FRAMES_RESET_SAMPLE 1
//the channel number used for determining samples
#define CHANNEL_NUMBER 1
//specific parameters to measure:
#define PARAM_MEASURE none

class AudioFilter {
public:
    AudioDecoder *ad = nullptr;
    AVFilterGraph *filterGraph = nullptr;
    //used to pass in the initial frame (the first 'filter' in the graph)
    AVFilterContext *srcContext = nullptr;
    const AVFilter *srcFilter = nullptr;


    AVFilterContext *lpContext = nullptr;
    const AVFilter *lpFilter = nullptr;

    AVFilterContext *hpContext = nullptr;
    const AVFilter *hpFilter = nullptr;

    AVFilterContext *arnndnContext = nullptr;
    const AVFilter *arnndnFilter = nullptr;

    //silence detector:
    AVFilterContext *silenceRemoverContext = nullptr;
    const AVFilter *silenceRemoverFilter = nullptr;

    AVFilterContext *beforeStatsContext = nullptr;
    const AVFilter *beforeStatsFilter = nullptr;

    AVFilterContext *afterStatsContext = nullptr;
    const AVFilter *afterStatsFilter = nullptr;

    //vad filter
    AVFilterContext *volumeContext = nullptr;
    const AVFilter *volumeFilter = nullptr;

    //the end point of the filter - this is how you specify what to write
    AVFilterContext *sinkContext = nullptr;
    const AVFilter *sinkFilter = nullptr;

    AVFilterContext *aFormatContext = nullptr;
    const AVFilter *aFormatFilter = nullptr;

    /**
     * initialize the audio filter with the parameters from the codec by the audio decoder object
     * @param ad the decoder
     */
    AudioFilter(AudioDecoder *ad);

    /*
     * creates a filter graph context
     * initilizes each filter with its parameters
     * links the filters together in a specific order:
     * src -> volume -> before Stats -> low pass -> high pass -> arnndn ->
     * silenceremover -> after stats -> aformat -> sink
     * configures the filtegraph
     */
    int initializeAllObjets();

    /**
     * frees the filtergraph context
     */
    void closeAllObjects();

private:

    /*https://ffmpeg.org/ffmpeg-filters.html#abuffer
     *creates the input filter: used as head node
     * recieves the frames
     */
    int initSrcFilter();

    /**https://ffmpeg.org/ffmpeg-filters.html#abuffersink
     *creates the output filter: used as tail node
     * outputs the processed frames after they passed thorugh the filtergraph
     * @return whether have an error during init
     */
    int initSinkFilter();

    /**
     *https://ffmpeg.org/ffmpeg-filters.html#volume
     * sets the output volume
     * @return whether have an error during init
     */
    int initVolumeFilter();

    /**
     *https://ffmpeg.org/ffmpeg-filters.html#lowpass
     * creates the low pass filter
     * @return whether have an error during init
     */
    int initLpFilter();

    /**
     * https://ffmpeg.org/ffmpeg-filters.html#highpass
     *creates the high pass filter
     * @return whether have an error during init
     */
    int initHpFilter();

    /**
     *creates the noise remover
     * model selection: beguiling-drafter bc the file is a recording, and we want to remove both noise and non-speech human sounds
     * more info can be found here: https://github.com/GregorR/rnnoise-models
     * need to provide the rnnn node file parameters
     * @return whether have an error during init
     */
    int initArnndnFilter();

    /**
     *https://ffmpeg.org/ffmpeg-filters.html#aformat-1
     * sets the output format of the filter
     * NOTE: does not specify the format of the demuxer: need 2nd loop to set header and tail after applying filter
     * @return whether have an error during init
     */
    int initFormatFilter();

    /**
     * https://ffmpeg.org/ffmpeg-filters.html#astat
     * measures the statistics -> mostly the rms during a specific frame
     * important: have parameters as need 2 filters: before and after running the filters
     * same filter, just need 2 different contexts
     * initialized with before stats and after stats
     * NOTE: specify parameters at the top of this file
     * @param afc the specific context of the filter (input context vs output context)
     * @param f the specific filter
     * @return whether have an error during init
     */
    int initStatFilter(AVFilterContext **afc, const AVFilter **f);

    /**
     *https://ffmpeg.org/ffmpeg-filters.html#silenceremove
     * initializes the silence remove filter
     * runs after the lp and hp filters
     * initializes both removal at the start and removal at the end of an audio file
     * NOTE: specify parameters at the top of this file
     * @return whether have an error during init
     */
    int initSilenceRemoverFilter();

    /**
     * a general initialization of the filter
     * gets the filter by name
     * allocates the filter to the filtergraph
     * @param af the specific context object of the filter
     * @param f the specifc filter object
     * @param name the name of the filter in the ffmpeg filter
     * @return whether have an error during init
     */
    int generalFilterInit(AVFilterContext **af, const AVFilter **f, const char *name) const;

    /**
     * initializes a specific filter parameter with an AVDictionary
     * give it the key and the value, and it specifies them in the specific filter
     * @param afc the specific context for which the parameters should be linked to
     * @param key the varialbe of the parameter
     * @param val the value that the variable should be set to
     * NOTE: check filter documentation for the specifc paramters and the acceptable range of keys
     * NO ERROR CHECK IS DONE - if a value is out of bounds, it will throw an error, but not specify why it was given
     * @return whether have an error during init
     */
    static int initByDict(AVFilterContext *afc, const char *key, const char *val);

    /**
     * initializes a filter based on specific functions
     * only used to initialize the source filter -> set the metadata (frame rate, bit rate, etc)
     * @param afc
     */
    void initByFunctions(AVFilterContext *afc) const;
};


#endif //FFMPEGTEST5_AUDIOFILTER_H