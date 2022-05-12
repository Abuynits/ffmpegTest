//
// Created by Alexiy Buynitsky on 4/29/22.
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
#define START_SILENCE 0.9
//HERE: STILL VARIABLE::
//The amount of time used for determining whether you have silence in audio. default =0.02,
#define WINDOW 0.9
//set how detect silence: "peak" = faster and works better with digital silence, "rms" = default
#define DETECTION rms
//used to skip over wanted silence. very important for skipping over gaps
#define STOP_DURATION 0

#include "AudioFilter.h"

int AudioFilter::initializeAllObjets() {
//filtergraph houses all of our filters that we will use
    filterGraph = avfilter_graph_alloc();
    if (filterGraph == nullptr) {
        cout << "ERROR: Unable to create filterGraph" << endl;
        return AVERROR(ENOMEM);
    }

    //----------------SRC FILTER CREATION----------------
    if (initSrcFilter() < 0) return -1;
    //----------------Volume FILTER CREATION----------------
    if (initVolumeFilter() < 0) return -1;
    //----------------Volume FILTER CREATION----------------
    if (initFormatFilter() < 0) return -1;
    //----------------low pass FILTER CREATION----------------
    if (initLpFilter() < 0) return -1;
    //----------------high pass FILTER CREATION----------------
    if (initHpFilter() < 0) return -1;
    //----------------arnndn FILTER CREATION----------------
    if (initArnndnFilter() < 0) return -1;
    //----------------arnndn FILTER CREATION----------------
    if (initSilenceRemoverFilter() < 0) return -1;
    //----------------SINK FILTER CREATION----------------
    if (initSinkFilter() < 0) return -1;
    /*
     * NOTES:
     * use arnndn filter which uses rnn, give it a model
     * https://github.com/GregorR/rnnoise-models
     */

//------------CONNECT THE FILTERS ------------------
    int resp;
    resp = avfilter_link(srcFilterContext, 0, volumeFilterContext, 0);
    if (resp >= 0) {
        resp = avfilter_link(volumeFilterContext, 0, lpFilterContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(lpFilterContext, 0, hpFilterContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(hpFilterContext, 0, arnndnFilterContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(arnndnFilterContext, 0, silenceRemoverFilterContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(silenceRemoverFilterContext, 0, aFormatContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(aFormatContext, 0, sinkFilterContext, 0);
    }
    if (resp < 0) {
        cout << "Error connecting filters: " << av_err2str(resp) << endl;
        return resp;
    }
    cout << "linked filters!" << endl;

    resp = avfilter_graph_config(filterGraph, nullptr);
    if (resp < 0) {
        cout << "ERROR: cannot configure filter graph " << av_err2str(resp) << endl;
        return resp;
    }
    cout << "configured graph!" << endl;

    return 0;
}

int AudioFilter::initSrcFilter() {
    int resp = generalFilterInit(&srcFilterContext, &srcFilter, "abuffer");
    if (resp < 0) {
        return resp;
    }
//check if have channel layout:
    if (!ad->pCodecContext->channel_layout) {
        cout << "warning: channel context not initialized... initializing" << endl;
        ad->pCodecContext->channel_layout = av_get_default_channel_layout(ad->pCodecContext->channels);
    }

    initByFunctions(srcFilterContext);

    resp = avfilter_init_str(srcFilterContext, nullptr);
    if (resp < 0) {
        cout << "ERROR: creating srcFilter: " << av_err2str(resp) << endl;
        return resp;
    }
    cout << "created srcFilter!" << endl;

    return 0;
}

int AudioFilter::initSinkFilter() {

    int resp = generalFilterInit(&sinkFilterContext, &sinkFilter, "abuffersink");
    if (resp < 0) {
        return resp;
    }

    resp = avfilter_init_str(sinkFilterContext, nullptr);
    if (resp < 0) {
        cout << "ERROR: creating sinkFilter: " << av_err2str(resp) << endl;
        return resp;
    }
    cout << "Created sink Filter!" << endl;
    return 0;
}

int AudioFilter::initVolumeFilter() {
    int resp = generalFilterInit(&volumeFilterContext, &volumeFilter, "volume");
    if (resp < 0) {
        return resp;
    }
    char *val = AV_STRINGIFY(VOLUME);
    resp = initByDict(volumeFilterContext, "volume", val);

    if (resp < 0) {
        fprintf(stderr, "Could not initialize the volume filter.\n");
        return resp;
    }

    cout << "created volume Filter!" << endl;
    return 0;
}

int AudioFilter::initFormatFilter() {
    int resp = generalFilterInit(&aFormatContext, &aFormatFilter, "aformat");
    if (resp < 0) {
        return resp;
    }

    char args[1024];
    snprintf(args, sizeof(args),
             "sample_fmts=%s:sample_rates=%d:channel_layouts=0x%" PRIx64,
             av_get_sample_fmt_name(ad->pCodecContext->sample_fmt), ad->pCodecContext->sample_rate,
             (uint64_t) (ad->pCodecContext->channel_layout));
    resp = avfilter_init_str(aFormatContext, args);
    if (resp < 0) {
        cout << "Could not initialize format filter context: " << av_err2str(AV_LOG_ERROR) << endl;
        return resp;
    }
    cout << "Created format Filter!" << endl;
    return resp;
}

int AudioFilter::initLpFilter() {
    int resp = generalFilterInit(&lpFilterContext, &lpFilter, "lowpass");
    if (resp < 0) {
        return resp;
    }
    char *val = AV_STRINGIFY(LOWPASS_VAL);
    resp = initByDict(lpFilterContext, "frequency", val);
    cout << "Created lp Filter!" << endl;
    return resp;
}

int AudioFilter::initHpFilter() {
    int resp = generalFilterInit(&hpFilterContext, &hpFilter, "highpass");
    if (resp < 0) {
        return resp;
    }
    char *val = AV_STRINGIFY(HIGHPASS_VAL);
    resp = initByDict(hpFilterContext, "frequency", val);
    cout << "Created hp Filter!" << endl;
    return resp;
}

int AudioFilter::initSilenceRemoverFilter() {
    int resp = generalFilterInit(&silenceRemoverFilterContext, &silenceRemoverFilter, "silenceremove");
    if (resp < 0) {
        return resp;
    }

//=========GENERAL INIT====================

    char *val = AV_STRINGIFY(START_PERIOD);
    resp = initByDict(silenceRemoverFilterContext, "start_periods", val);
    if (resp < 0) {
        return resp;
    }
    val = AV_STRINGIFY(STOP_PERIOD);
    resp = initByDict(silenceRemoverFilterContext, "stop_periods", val);
    if (resp < 0) {
        return resp;
    }
    val = AV_STRINGIFY(WINDOW);
    resp = initByDict(silenceRemoverFilterContext, "window", val);
    if (resp < 0) {
        return resp;
    }
    val = AV_STRINGIFY(DETECTION);
    resp = initByDict(silenceRemoverFilterContext, "detection", val);
    if (resp < 0) {
        return resp;
    }
//=========START FILTERS====================


    val = AV_STRINGIFY(START_THRESHOLD);
    resp = initByDict(silenceRemoverFilterContext, "start_threshold", val);
    if (resp < 0) {
        return resp;
    }

    val = AV_STRINGIFY(START_SILENCE);
    resp = initByDict(silenceRemoverFilterContext, "start_silence", val);
    if (resp < 0) {
        return resp;
    }

//=========END FILTERS====================

    val = AV_STRINGIFY(START_DURATION);
    resp = initByDict(silenceRemoverFilterContext, "start_duration", val);
    if (resp < 0) {
        return resp;
    }
    val = AV_STRINGIFY(STOP_THRESHOLD);
    resp = initByDict(silenceRemoverFilterContext, "stop_threshold", val);
    if (resp < 0) {
        return resp;
    }
    //stop_periods=-1:stop_duration=1:stop_threshold=-90dB

    val = AV_STRINGIFY(STOP_SILENCE);
    resp = initByDict(silenceRemoverFilterContext, "stop_silence", val);
    if (resp < 0) {
        return resp;
    }

    val = AV_STRINGIFY(STOP_DURATION);
    resp = initByDict(silenceRemoverFilterContext, "stop_duration", val);
    if (resp < 0) {
        return resp;
    }
    cout << "Created silenceremove Filter!" << endl;
    //use these two values to specify the actual decibal value which is considered silence or not
    //TODO: use https://ffmpeg.org/ffmpeg-filters.html#silencedetect

    return resp;

}

int AudioFilter::initArnndnFilter() {
    int resp = generalFilterInit(&arnndnFilterContext, &arnndnFilter, "arnndn");
    if (resp < 0) {
        return resp;
    }
    /*
     * model selection: beguiling-drafter bc the file is a recording, and we want to remove both noise and non-speech human sounds
     * more info can be found here: https://github.com/GregorR/rnnoise-models
     */
    char *val = "/Users/abuynits/CLionProjects/ffmpegTest5/rnnoise-models-master/beguiling-drafter-2018-08-30/bd.rnnn";

    resp = initByDict(arnndnFilterContext, "model", val);
    cout << "Created arnndn Filter!" << endl;
    return resp;
}

AudioFilter::AudioFilter(AudioDecoder *ad) {
    this->ad = ad;
}

void AudioFilter::closeAllObjects() {
    avfilter_graph_free(&filterGraph);
}

int AudioFilter::initByDict(AVFilterContext *afc, const char *key, const char *val) {

    AVDictionary *optionsDict = nullptr;
    av_dict_set(&optionsDict, key, val, 0);

    int resp = avfilter_init_dict(afc, &optionsDict);
    av_dict_free(&optionsDict);

    return resp;
}

void AudioFilter::initByFunctions(AVFilterContext *afc) const {
    char ch_layout[64];
    av_get_channel_layout_string(ch_layout, sizeof(ch_layout), 0, ad->pCodecContext->channel_layout);
    av_opt_set(afc, "channel_layout", ch_layout, AV_OPT_SEARCH_CHILDREN);
    av_opt_set(afc, "sample_fmt", av_get_sample_fmt_name(ad->pCodecContext->sample_fmt),
               AV_OPT_SEARCH_CHILDREN);
    av_opt_set_q(afc, "time_base", (AVRational) {1, ad->pCodecContext->sample_rate},
                 AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(afc, "sample_rate", ad->pCodecContext->sample_rate, AV_OPT_SEARCH_CHILDREN);
}

int AudioFilter::generalFilterInit(AVFilterContext **af, const AVFilter **f, const char *name) const {
    *f = avfilter_get_by_name(name);
    if (*f == nullptr) {
        cout << "Could not find " << name << " filter: " << av_err2str(AVERROR_FILTER_NOT_FOUND) << endl;
        return AVERROR_FILTER_NOT_FOUND;
    }

    *af = avfilter_graph_alloc_filter(filterGraph, *f, name);
    if (*af == nullptr) {
        cout << "Could not allocate " << name << " filter context: " << av_err2str(AVERROR(ENOMEM)) << endl;
        return AVERROR(ENOMEM);
    }
    return 0;
}

