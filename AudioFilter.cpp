//
// Created by Alexiy Buynitsky on 4/29/22.
//
#define VOLUME 1.00
#define LOWPASS_VAL 3000
#define HIGHPASS_VAL 200
#define SILENCE_DB -30dB
#define SILENCE_FRAMES 1
#define WANTED_SILENCE 0.2
#define NUMBER_FILTERS 4

#include <list>
#include "AudioFilter.h"

int AudioFilter::initializeAllObjets() {
//filtergraph houses all of our filters that we will use
    filterGraph = avfilter_graph_alloc();
    if (filterGraph == nullptr) {
        cout << "ERROR: Unable to create filterGraph" << endl;
        return AVERROR(ENOMEM);
    }

    InnerFilter filters[NUMBER_FILTERS];
    string filterName = "volume";
    string keys = "volume";
    string vals = AV_STRINGIFY(VOLUME);
    filters[0] = InnerFilter(this, filterName, keys, vals);

    filterName = "lowpass";
    keys = "frequency";
    vals = AV_STRINGIFY(LOWPASS_VAL);
    filters[1] = InnerFilter(this, filterName, keys, vals);

    filterName = "highpass";
    keys = "frequency";
    vals = AV_STRINGIFY(HIGHPASS_VAL);
    filters[2] = InnerFilter(this, filterName, keys, vals);

    filterName = "arnndn";
    keys = "model";
    vals = {"/Users/abuynits/CLionProjects/ffmpegTest5/rnnoise-models-master/beguiling-drafter-2018-08-30/bd.rnnn"};
    filters[3] = InnerFilter(this, filterName, keys, vals);

//    filterName = "silenceremove";
//    string multKeys[] = {"window", "start_periods", "start_threshold", "start_duration", "start_silence",
//                         "stop_periods",
//                         "stop_threshold", "stop_duration"};
//    string multVals[] = {AV_STRINGIFY(0.3), AV_STRINGIFY(1), AV_STRINGIFY(-30dB), AV_STRINGIFY(0.2),
//                         AV_STRINGIFY(WANTED_SILENCE),
//                         AV_STRINGIFY(1), AV_STRINGIFY(-40dB), AV_STRINGIFY(100)};
//    filters[4] = InnerFilter(this, filterName, multKeys, multVals, 1);

//    for (InnerFilter f: filters) {
//        int resp = f.initFilter();
//        if (resp < 0) {
//            cout << "error in init filter!" << endl;
//            return -1;
//        }
//    }


//    //----------------SRC FILTER CREATION----------------
//    if (initSrcFilter() < 0) return -1;
//    //----------------Volume FILTER CREATION----------------
//    if (initVolumeFilter() < 0) return -1;
//    //----------------Volume FILTER CREATION----------------
//    if (initFormatFilter() < 0) return -1;
//    //----------------low pass FILTER CREATION----------------
//    if (initLpFilter() < 0) return -1;
//    //----------------high pass FILTER CREATION----------------
//    if (initHpFilter() < 0) return -1;
//    //----------------arnndn FILTER CREATION----------------
//    if (initArnndnFilter() < 0) return -1;
//    //----------------arnndn FILTER CREATION----------------
//    if (initSilenceRemoverFilter() < 0) return -1;
//    //----------------SINK FILTER CREATION----------------
//    if (initSinkFilter() < 0) return -1;
//    /*
//     * NOTES:
//     * use arnndn filter which uses rnn, give it a model
//     * https://github.com/GregorR/rnnoise-models
//     */

//------------CONNECT THE FILTERS ------------------

    int resp;


    resp = avfilter_link(srcFilterContext, 0, filters[0].filterContext, 0);
    if (resp < 0) {
        goto exitLinking;
    }
    cout<<"here"<<endl;
    for (int i = 1; i < NUMBER_FILTERS - 1; i++) {
        resp = avfilter_link(filters[i].filterContext, 0, filters[i + 1].filterContext, 0);
        if (resp < 0) {
            cout << "error on: " << i << endl;
            goto exitLinking;
        }

    }
    if (resp >= 0) {
        resp = avfilter_link(filters[NUMBER_FILTERS - 1].filterContext, 0, sinkFilterContext, 0);
    }
    exitLinking:
    if (resp < 0) {
        cout << "Error connecting filters: " << av_err2str(resp) << endl;
        return resp;
    }
    /*
     *
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
     */
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
    //create aBuffer filter, used for inputing data to filtergraph -> recieves frames from the decoder
    srcFilter = avfilter_get_by_name("abuffer");
    if (srcFilter == nullptr) {
        cout << "ERROR: Could not find the abuffer filter" << endl;

        return AVERROR_FILTER_NOT_FOUND;
    }
    srcFilterContext = avfilter_graph_alloc_filter(filterGraph, srcFilter, "src");
    if (srcFilterContext == nullptr) {
        cout << "Could not allocate the inputFilter context" << endl;
        return AVERROR(ENOMEM);
    }

//check if have channel layout:
    if (!ad->pCodecContext->channel_layout) {
        cout << "warning: channel context not initialized... initializing" << endl;
        ad->pCodecContext->channel_layout = av_get_default_channel_layout(ad->pCodecContext->channels);
    }

    initByFunctions(srcFilterContext);

    int resp = avfilter_init_str(srcFilterContext, nullptr);
    if (resp < 0) {
        cout << "ERROR: creating srcFilter: " << av_err2str(resp) << endl;
        return resp;
    }
    cout << "created srcFilter!" << endl;

    return 0;
}

int AudioFilter::initSinkFilter() {

    int resp;

    sinkFilter = avfilter_get_by_name("abuffersink");
    if (srcFilter == nullptr) {
        cout << "ERROR: Could not find the abuffersink filter" << endl;

        return AVERROR_FILTER_NOT_FOUND;
    }
    sinkFilterContext = avfilter_graph_alloc_filter(filterGraph, sinkFilter, "sink");
    if (sinkFilterContext == nullptr) {
        cout << "Could not allocate the outputFilter context" << endl;
        return AVERROR(ENOMEM);
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
    volumeFilter = avfilter_get_by_name("volume");
    if (volumeFilter == nullptr) {
        cout << "Could not find the volume filter: " << av_err2str(AVERROR_FILTER_NOT_FOUND) << endl;
        return AVERROR_FILTER_NOT_FOUND;
    }

    volumeFilterContext = avfilter_graph_alloc_filter(filterGraph, volumeFilter, "volume");
    if (volumeFilterContext == nullptr) {
        cout << "Could not find the volume filter: " << av_err2str(AVERROR_FILTER_NOT_FOUND) << endl;
        return AVERROR_FILTER_NOT_FOUND;
    }

    char *val = AV_STRINGIFY(VOLUME);
    int resp = initByDict(volumeFilterContext, "volume", val);

    if (resp < 0) {
        fprintf(stderr, "Could not initialize the volume filter.\n");
        return resp;
    }

    cout << "created volume Filter!" << endl;
    return 0;
}

int AudioFilter::initFormatFilter() {

    int resp;
    aFormatFilter = avfilter_get_by_name("aformat");
    if (aFormatFilter == nullptr) {
        cout << "Could not find aformat filter: " << av_err2str(AVERROR_FILTER_NOT_FOUND) << endl;
        return AVERROR_FILTER_NOT_FOUND;
    }

    aFormatContext = avfilter_graph_alloc_filter(filterGraph, aFormatFilter, "aformat");
    if (aFormatContext == nullptr) {
        cout << "Could not allocate format filter context: " << av_err2str(AVERROR(ENOMEM)) << endl;
        return AVERROR(ENOMEM);
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
    int resp;
    lpFilter = avfilter_get_by_name("lowpass");
    if (lpFilter == nullptr) {
        cout << "Could not find lowpass filter: " << av_err2str(AVERROR_FILTER_NOT_FOUND) << endl;
        return AVERROR_FILTER_NOT_FOUND;
    }

    lpFilterContext = avfilter_graph_alloc_filter(filterGraph, lpFilter, "lowpass");
    if (lpFilterContext == nullptr) {
        cout << "Could not allocate format filter context: " << av_err2str(AVERROR(ENOMEM)) << endl;
        return AVERROR(ENOMEM);
    }
    char *val = AV_STRINGIFY(LOWPASS_VAL);
    resp = initByDict(lpFilterContext, "frequency", val);
    return resp;
}

int AudioFilter::initHpFilter() {
    int resp;
    hpFilter = avfilter_get_by_name("highpass");
    if (hpFilter == nullptr) {
        cout << "Could not find highpass filter: " << av_err2str(AVERROR_FILTER_NOT_FOUND) << endl;
        return AVERROR_FILTER_NOT_FOUND;
    }

    hpFilterContext = avfilter_graph_alloc_filter(filterGraph, hpFilter, "highpass");
    if (hpFilterContext == nullptr) {
        cout << "Could not allocate format filter context: " << av_err2str(AVERROR(ENOMEM)) << endl;
        return AVERROR(ENOMEM);
    }
    char *val = AV_STRINGIFY(HIGHPASS_VAL);
    resp = initByDict(hpFilterContext, "frequency", val);
    return resp;
}

int AudioFilter::initSilenceRemoverFilter() {
    int resp;
    silenceRemoverFilter = avfilter_get_by_name("silenceremove");
    if (silenceRemoverFilter == nullptr) {
        cout << "Could not find highpass filter: " << av_err2str(AVERROR_FILTER_NOT_FOUND) << endl;
        return AVERROR_FILTER_NOT_FOUND;
    }

    silenceRemoverFilterContext = avfilter_graph_alloc_filter(filterGraph, silenceRemoverFilter, "silenceremove");
    if (silenceRemoverFilterContext == nullptr) {
        cout << "Could not allocate format filter context: " << av_err2str(AVERROR(ENOMEM)) << endl;
        return AVERROR(ENOMEM);
    }
    char *val;

    //===============General Setup====================
    val = AV_STRINGIFY(0.3);
    resp = initByDict(silenceRemoverFilterContext, "window", val);
    if (resp < 0) {
        return resp;
    }

    //===============START OF AUDIO====================
    val = AV_STRINGIFY(1);
    resp = initByDict(silenceRemoverFilterContext, "start_periods", val);
    if (resp < 0) {
        return resp;
    }
    val = AV_STRINGIFY(-30dB);
    resp = initByDict(silenceRemoverFilterContext, "start_threshold", val);
    if (resp < 0) {
        return resp;
    }
    val = AV_STRINGIFY(0.2);
    resp = initByDict(silenceRemoverFilterContext, "start_duration", val);
    if (resp < 0) {
        return resp;
    }
    val = AV_STRINGIFY(WANTED_SILENCE);
    resp = initByDict(silenceRemoverFilterContext, "start_silence", val);
    if (resp < 0) {
        return resp;
    }
    //===============End OF AUDIO====================


    val = AV_STRINGIFY(1);
    resp = initByDict(silenceRemoverFilterContext, "stop_periods", val);
    if (resp < 0) {
        return resp;
    }

    val = AV_STRINGIFY(-40dB);
    resp = initByDict(silenceRemoverFilterContext, "stop_threshold", val);
    if (resp < 0) {
        return resp;
    }
    //stop_periods=-1:stop_duration=1:stop_threshold=-90dB

    val = AV_STRINGIFY(100);
    resp = initByDict(silenceRemoverFilterContext, "stop_duration", val);
    if (resp < 0) {
        return resp;
    }


    return resp;

    //===============START OF AUDIO====================

    val = AV_STRINGIFY(1);
    resp = initByDict(silenceRemoverFilterContext, "start_periods", val);
    if (resp < 0) {
        return resp;
    }
    val = AV_STRINGIFY(-30dB);
    resp = initByDict(silenceRemoverFilterContext, "start_threshold", val);
    if (resp < 0) {
        return resp;
    }
    val = AV_STRINGIFY(0.2);
    resp = initByDict(silenceRemoverFilterContext, "start_silence", val);
    if (resp < 0) {
        return resp;
    }

    //==========END OF AUDIO=================

//
//    val = AV_STRINGIFY(1);
//    resp = initByDict(silenceRemoverFilterContext, "stop_periods", val);
//    if (resp < 0) {
//        return resp;
//    }
//    val = AV_STRINGIFY(-40dB);
//    resp = initByDict(silenceRemoverFilterContext, "stop_threshold", val);
//    if (resp < 0) {
//        return resp;
//    }
//    //stop_periods=-1:stop_duration=1:stop_threshold=-90dB
//
//    val = AV_STRINGIFY(1);
//    resp = initByDict(silenceRemoverFilterContext, "stop_duration", val);
//    if (resp < 0) {
//        return resp;
//    }
//    //TODO: bug here with trimming a value
//    val = AV_STRINGIFY(0.5);
//    resp = initByDict(silenceRemoverFilterContext, "stop_silence", val);
//    if (resp < 0) {
//        return resp;
//    }


    //start_threshold
    //stop_threshold
    //use these two values to specify the actual decibal value which is considered silence or not
    //TODO: use https://ffmpeg.org/ffmpeg-filters.html#silencedetect

    return resp;

}

int AudioFilter::initArnndnFilter() {
    int resp;
    arnndnFilter = avfilter_get_by_name("arnndn");
    if (arnndnFilter == nullptr) {
        cout << "Could not find arnndn filter: " << av_err2str(AVERROR_FILTER_NOT_FOUND) << endl;
        return AVERROR_FILTER_NOT_FOUND;
    }

    arnndnFilterContext = avfilter_graph_alloc_filter(filterGraph, arnndnFilter, "arnndn");
    if (arnndnFilterContext == nullptr) {
        cout << "Could not allocate arnndn filter context: " << av_err2str(AVERROR(ENOMEM)) << endl;
        return AVERROR(ENOMEM);
    }
    /*
     * model selection: beguiling-drafter bc the file is a recording, and we want to remove both noise and non-speech human sounds
     * more info can be found here: https://github.com/GregorR/rnnoise-models
     */
    char *val = "/Users/abuynits/CLionProjects/ffmpegTest5/rnnoise-models-master/beguiling-drafter-2018-08-30/bd.rnnn";

    resp = initByDict(arnndnFilterContext, "model", val);
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

void AudioFilter::initByFunctions(AVFilterContext *afc) {
    char ch_layout[64];
    av_get_channel_layout_string(ch_layout, sizeof(ch_layout), 0, ad->pCodecContext->channel_layout);
    av_opt_set(afc, "channel_layout", ch_layout, AV_OPT_SEARCH_CHILDREN);
    av_opt_set(afc, "sample_fmt", av_get_sample_fmt_name(ad->pCodecContext->sample_fmt),
               AV_OPT_SEARCH_CHILDREN);
    av_opt_set_q(afc, "time_base", (AVRational) {1, ad->pCodecContext->sample_rate},
                 AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(afc, "sample_rate", ad->pCodecContext->sample_rate, AV_OPT_SEARCH_CHILDREN);
}

int AudioFilter::InnerFilter::initSingleInputFilter() {
    filter = avfilter_get_by_name(name.c_str());
    if (filter == nullptr) {
        cout << "Could not find the volume filter: " << av_err2str(AVERROR_FILTER_NOT_FOUND) << endl;
        return AVERROR_FILTER_NOT_FOUND;
    }

    filterContext = avfilter_graph_alloc_filter(av->filterGraph, filter, name.c_str());
    if (filterContext == nullptr) {
        cout << "Could not find the " << name << " filter: " << av_err2str(AVERROR_FILTER_NOT_FOUND) << endl;
        return AVERROR_FILTER_NOT_FOUND;
    }
    int resp = initByDict(key.c_str(), value.c_str());
    if (resp < 0) {
        cout << "Could not initialize the " << name << " with " << key.c_str() << " and " << value.c_str() << endl;
        return resp;
    }


    cout << "created " << name << " Filter!" << endl;
    return 0;
}

int AudioFilter::InnerFilter::initByDict(const char *key, const char *val) const {

    AVDictionary *optionsDict = nullptr;
    av_dict_set(&optionsDict, key, val, 0);

    int resp = avfilter_init_dict(filterContext, &optionsDict);
    av_dict_free(&optionsDict);

    return resp;
}

int AudioFilter::InnerFilter::initMultipleInputFilter() {
    filter = avfilter_get_by_name(name.c_str());
    if (filter == nullptr) {
        cout << "Could not find the volume filter: " << av_err2str(AVERROR_FILTER_NOT_FOUND) << endl;
        return AVERROR_FILTER_NOT_FOUND;
    }

    filterContext = avfilter_graph_alloc_filter(av->filterGraph, filter, name.c_str());
    if (filterContext == nullptr) {
        cout << "Could not find the " << name << " filter: " << av_err2str(AVERROR_FILTER_NOT_FOUND) << endl;
        return AVERROR_FILTER_NOT_FOUND;
    }
    for (int i = 0; i < numFactors; i++) {
        const char *k = keys[i].c_str();
        const char *v = values[i].c_str();

        int resp = initByDict(k, v);
        if (resp < 0) {
            cout << "Could not initialize the " << name << " with " << k << " and " << v << endl;
            return resp;
        }
    }


    cout << "created " << name << " Filter!" << endl;
    return 0;

    return 0;
}
