//
// Created by Alexiy Buynitsky on 4/29/22.

#include "AudioFilter.h"

int AudioFilter::initializeAllObjets() {
//filtergraph houses all of our filters that we will use
    filterGraph = avfilter_graph_alloc();
    if (filterGraph == nullptr) {
        cerr << "ERROR: Unable to create filterGraph" << endl;
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
    //----------------before stats FILTER CREATION----------------
    if (initStatFilter(&beforeStatsContext, &beforeStatsFilter) < 0) return -1;
    //----------------after stats FILTER CREATION----------------
    if (initStatFilter(&afterStatsContext, &afterStatsFilter) < 0) return -1;
    //----------------silenceremove FILTER CREATION----------------
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
    resp = avfilter_link(srcContext, 0, volumeContext, 0);
    if (resp >= 0) {
        resp = avfilter_link(volumeContext, 0, beforeStatsContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(beforeStatsContext, 0, lpContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(lpContext, 0, hpContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(hpContext, 0, arnndnContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(arnndnContext, 0, silenceRemoverContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(silenceRemoverContext, 0, afterStatsContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(afterStatsContext, 0, aFormatContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(aFormatContext, 0, sinkContext, 0);
    }
    if (resp < 0) {
        cerr << "Error connecting filters: " << av_err2str(resp) << endl;
        return resp;
    }

    resp = avfilter_graph_config(filterGraph, nullptr);
    if (resp < 0) {
        cerr << "ERROR: cannot configure filter graph " << av_err2str(resp) << endl;
        return resp;
    }
    cerr << "configured graph!" << endl;

    return 0;
}

int AudioFilter::initSrcFilter() {
    int resp = generalFilterInit(&srcContext, &srcFilter, "abuffer");
    if (resp < 0) {
        return resp;
    }
//check if have channel layout:
    if (!ad->pInCodecContext->channel_layout) {
        cerr << "\twarning: channel context not initialized... initializing" << endl;
        ad->pInCodecContext->channel_layout = av_get_default_channel_layout(ad->pInCodecContext->channels);
    }

    initByFunctions(srcContext);

    resp = avfilter_init_str(srcContext, nullptr);
    if (resp < 0) {
        cerr << "ERROR: creating srcFilter: " << av_err2str(resp) << endl;
        return resp;
    }
    cerr << "\tcreated srcFilter!" << endl;

    return 0;
}

int AudioFilter::initSinkFilter() {

    int resp = generalFilterInit(&sinkContext, &sinkFilter, "abuffersink");
    if (resp < 0) {
        return resp;
    }

    resp = avfilter_init_str(sinkContext, nullptr);
    if (resp < 0) {
        cerr << "ERROR: creating sinkFilter: " << av_err2str(resp) << endl;
        return resp;
    }
    cerr << "\tCreated sink Filter!" << endl;
    return 0;
}

int AudioFilter::initVolumeFilter() {
    int resp = generalFilterInit(&volumeContext, &volumeFilter, "volume");
    if (resp < 0) {
        return resp;
    }
    char *val = AV_STRINGIFY(VOLUME);
    resp = initByDict(volumeContext, "volume", val);

    if (resp < 0) {
        fprintf(stderr, "Could not initialize the volume filter.\n");
        return resp;
    }

    cerr << "\tcreated volume Filter!" << endl;
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
             av_get_sample_fmt_name(ad->pOutCodecContext->sample_fmt), ad->pOutCodecContext->sample_rate,
             (uint64_t) (ad->pOutCodecContext->channel_layout));
    resp = avfilter_init_str(aFormatContext, args);
    if (resp < 0) {
        cerr << "Could not initialize format filter context: " << av_err2str(AV_LOG_ERROR) << endl;
        return resp;
    }
    cerr << "\tCreated format Filter!" << endl;
    return resp;
}

int AudioFilter::initLpFilter() {
    int resp = generalFilterInit(&lpContext, &lpFilter, "lowpass");
    if (resp < 0) {
        return resp;
    }
    char *val = AV_STRINGIFY(LOWPASS_VAL);
    resp = initByDict(lpContext, "frequency", val);
    cerr << "\tCreated lp Filter!" << endl;
    return resp;
}

int AudioFilter::initHpFilter() {
    int resp = generalFilterInit(&hpContext, &hpFilter, "highpass");
    if (resp < 0) {
        return resp;
    }
    char *val = AV_STRINGIFY(HIGHPASS_VAL);
    resp = initByDict(hpContext, "frequency", val);
    cerr << "\tCreated hp Filter!" << endl;
    return resp;
}

int AudioFilter::initSilenceRemoverFilter() {
    int resp = generalFilterInit(&silenceRemoverContext, &silenceRemoverFilter, "silenceremove");
    if (resp < 0) {
        return resp;
    }

//=========GENERAL INIT====================

    char *val = AV_STRINGIFY(START_PERIOD);
    resp = initByDict(silenceRemoverContext, "start_periods", val);
    if (resp < 0) {
        return resp;
    }
    val = AV_STRINGIFY(STOP_PERIOD);
    resp = initByDict(silenceRemoverContext, "stop_periods", val);
    if (resp < 0) {
        return resp;
    }
    val = AV_STRINGIFY(WINDOW);
    resp = initByDict(silenceRemoverContext, "window", val);
    if (resp < 0) {
        return resp;
    }
    val = AV_STRINGIFY(DETECTION);
    resp = initByDict(silenceRemoverContext, "detection", val);
    if (resp < 0) {
        return resp;
    }
//=========START FILTERS====================


    val = AV_STRINGIFY(START_THRESHOLD);
    resp = initByDict(silenceRemoverContext, "start_threshold", val);
    if (resp < 0) {
        return resp;
    }

    val = AV_STRINGIFY(START_SILENCE);
    resp = initByDict(silenceRemoverContext, "start_silence", val);
    if (resp < 0) {
        return resp;
    }

//=========END FILTERS====================

    val = AV_STRINGIFY(START_DURATION);
    resp = initByDict(silenceRemoverContext, "start_duration", val);
    if (resp < 0) {
        return resp;
    }
    val = AV_STRINGIFY(STOP_THRESHOLD);
    resp = initByDict(silenceRemoverContext, "stop_threshold", val);
    if (resp < 0) {
        return resp;
    }
    //stop_periods=-1:stop_duration=1:stop_threshold=-90dB

    val = AV_STRINGIFY(STOP_SILENCE);
    resp = initByDict(silenceRemoverContext, "stop_silence", val);
    if (resp < 0) {
        return resp;
    }

    val = AV_STRINGIFY(STOP_DURATION);
    resp = initByDict(silenceRemoverContext, "stop_duration", val);
    if (resp < 0) {
        return resp;
    }
    cerr << "\tCreated silenceremove Filter!" << endl;
    //use these two values to specify the actual decibal value which is considered silence or not
    //TODO: use https://ffmpeg.org/ffmpeg-filters.html#silencedetect

    return resp;

}

int AudioFilter::initArnndnFilter() {
    int resp = generalFilterInit(&arnndnContext, &arnndnFilter, "arnndn");
    if (resp < 0) {
        return resp;
    }

    char *val = "/Users/abuynits/CLionProjects/ffmpegTest5/rnnoise-models-master/beguiling-drafter-2018-08-30/bd.rnnn";

    resp = initByDict(arnndnContext, "model", val);
    cerr << "\tCreated arnndn Filter!" << endl;
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
    av_get_channel_layout_string(ch_layout, sizeof(ch_layout), 0, ad->pInCodecContext->channel_layout);
    av_opt_set(afc, "channel_layout", ch_layout, AV_OPT_SEARCH_CHILDREN);
    av_opt_set(afc, "sample_fmt", av_get_sample_fmt_name(ad->pInCodecContext->sample_fmt),
               AV_OPT_SEARCH_CHILDREN);
    av_opt_set_q(afc, "time_base", (AVRational) {1, ad->pInCodecContext->sample_rate},
                 AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(afc, "sample_rate", ad->pInCodecContext->sample_rate, AV_OPT_SEARCH_CHILDREN);
}

int AudioFilter::generalFilterInit(AVFilterContext **af, const AVFilter **f, const char *name) const {
    *f = avfilter_get_by_name(name);
    if (*f == nullptr) {
        cerr << "Could not find " << name << " filter: " << av_err2str(AVERROR_FILTER_NOT_FOUND) << endl;
        return AVERROR_FILTER_NOT_FOUND;
    }

    *af = avfilter_graph_alloc_filter(filterGraph, *f, name);
    if (*af == nullptr) {
        cerr << "Could not allocate " << name << " filter context: " << av_err2str(AVERROR(ENOMEM)) << endl;
        return AVERROR(ENOMEM);
    }
    return 0;
}

int AudioFilter::initStatFilter(AVFilterContext **afc, const AVFilter **f) {
    int resp = generalFilterInit(afc, f, "astats");
    if (resp < 0) {
        return resp;
    }
    //TODO: need to tune these parameters
    //the number of frames after which recalculate stats
    char *val = AV_STRINGIFY(FRAMES_RESET_SAMPLE);
    resp = initByDict(*afc, "reset", val);
    if (resp < 0) {
        return resp;
    }
    //the lenght of audio sample used for determining stats: 0.05 = 5 milliseconds
    val = AV_STRINGIFY(AUDIO_SAMPLE_LENGTH);
    resp = initByDict(*afc, "length", val);
    if (resp < 0) {
        return resp;
    }
    //the channel number = should have 1 as default
    val = AV_STRINGIFY(CHANNEL_NUMBER);
    resp = initByDict(*afc, "metadata", val);
    if (resp < 0) {
        return resp;
    }
    //not track any parameters
    val = AV_STRINGIFY(PARAM_MEASURE);
    resp = initByDict(*afc, "measure_perchannel", val);
    if (resp < 0) {
        return resp;
    }


    return 0;
}