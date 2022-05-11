//
// Created by Alexiy Buynitsky on 4/29/22.
//
#define VOLUME 1.00
#define LOWPASS_VAL 3000
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
    //----------------SINK FILTER CREATION----------------
    if (initSinkFilter() < 0) return -1;

//------------CONNECT THE FILTERS ------------------
    int resp;
    resp = avfilter_link(srcFilterContext, 0, volumeFilterContext, 0);
    if (resp >= 0) {
        resp = avfilter_link(volumeFilterContext, 0, lpFilterContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(lpFilterContext, 0, aFormatContext, 0);
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

AudioFilter::AudioFilter(AudioDecoder *ad) {
    this->ad = ad;
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

void AudioFilter::closeAllObjects() {
    avfilter_graph_free(&filterGraph);
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

int AudioFilter::initLpFilter() {
    int resp;
    lpFilter = avfilter_get_by_name("lowpass");
    if (lpFilter == nullptr) {
        cout << "Could not find aformat filter: " << av_err2str(AVERROR_FILTER_NOT_FOUND) << endl;
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

