//
// Created by Alexiy Buynitsky on 4/29/22.
//

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
    //----------------SINK FILTER CREATION----------------
    if (initSinkFilter() < 0) return -1;
    //----------------Volume FILTER CREATION----------------
   // if (initVolumeFilter() < 0) return -1;

    //------------------------------------------

    int resp = 0;
//    const enum AVSampleFormat out_sample_fmts[] = {ad->pCodecContext->sample_fmt, AV_SAMPLE_FMT_NONE};
//    resp = av_opt_set_int_list((void *) sinkFilter, "sample_fmts", out_sample_fmts, -1,
//                               AV_OPT_SEARCH_CHILDREN);
//    if (resp < 0) {
//        cout << "ERROR: output format cannot be set: " << av_err2str(resp) << endl;
//        return resp;
//    }
//
//
//    resp = av_opt_set_int_list(sinkFilterContext, "channel_layouts", ad->pCodec->channel_layouts, -1,
//                               AV_OPT_SEARCH_CHILDREN);
//    if (resp < 0) {
//        cout << "ERROR: channel layout cannot be set: " << av_err2str(resp) << endl;
//        return resp;
//    }
//
//
//
////NOTE: using supported_samplerates
//
//    resp = av_opt_set_int_list(sinkFilterContext, "sample_rates", out_sample_fmts, -1,
//                               AV_OPT_SEARCH_CHILDREN);
//    if (resp < 0) {
//        cout << "ERROR: sample rates cannot be set: " << av_err2str(resp) << endl;
//        return resp;
//    }

////set the endpoints for the inputs and outputs of the filter
//    inputs = avfilter_inout_alloc();
//    outputs = avfilter_inout_alloc();
//
//    outputs->name = av_strdup("in");
//    outputs->filter_ctx = srcFilterContext;
//    outputs->pad_idx = 0;
//    outputs->next = nullptr;
//
//    inputs->name = av_strdup("out");
//    inputs->filter_ctx = sinkFilterContext;
//    inputs->pad_idx = 0;
//    inputs->next = nullptr;
//    resp = avfilter_graph_parse_ptr(filterGraph, filterDescription,
//                                    &inputs, &outputs, nullptr);
//    if (resp < 0) {
//        cout << "ERROR: cannot add graph described by input " << av_err2str(resp) << endl;
//        return resp;
//    }
//------------CONNECT THE FILTERS ------------------

    resp = avfilter_link(srcFilterContext, 0, sinkFilterContext, 0);
    if (resp >= 0) {
       // resp = avfilter_link(volumeFilterContext, 0, sinkFilterContext, 0);
    }
    if (resp < 0) {
        cout << "Error connecting filters: " << av_err2str(resp) << endl;
        return resp;
    }
    resp = avfilter_graph_config(filterGraph, nullptr);
    if (resp < 0) {
        cout << "ERROR: cannot configure filter graph " << av_err2str(resp) << endl;
        return resp;
    }
    cout<<"Linked filters!"<<endl;

//    outlink = sinkFilterContext->inputs[0];
//    av_get_channel_layout_string(args, sizeof(args), -1, outlink->channel_layout);
    resp = avfilter_graph_config(filterGraph, nullptr);
    if (resp < 0) {
        cout<<"Error configuring the filter graph: "<<av_err2str(resp);
        return resp;
    }
    cout<<"configured graph!"<<endl;
    
//    av_log(nullptr, AV_LOG_INFO, "Output: srate:%dHz fmt:%s chlayout:%s\n",
//           (int) outlink->sample_rate,
//           (char *) av_x_if_null(av_get_sample_fmt_name(static_cast<AVSampleFormat>(outlink->format)), "?"),
//           args);
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
//OTHER ATTEMPTS AT SETTING CHANNEL
// NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE NIGHTMARE
    //   AVRational time_base = ad->pFormatContext->streams[audio_stream_index]->time_base;
////store the value in args as if you were going to print.
//    snprintf(args, sizeof(args),
//             "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=",
//             time_base.num, time_base.den, ad->pCodecContext->sample_rate,
//             av_get_sample_fmt_name(ad->pCodecContext->sample_fmt));
//    snprintf(args, sizeof(args),
//             "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%"
//             PRIx64,
//             time_base.num, time_base.den, ad->pCodecContext->sample_rate,
//             av_get_sample_fmt_name(ad->pCodecContext->sample_fmt), ad->pCodecContext->channel_layout);

    /* Set the filter options through the AVOptions API. */

    char ch_layout[64];
    av_get_channel_layout_string(ch_layout, sizeof(ch_layout), 0, ad->pCodecContext->channel_layout);
    av_opt_set(srcFilterContext, "channel_layout", ch_layout, AV_OPT_SEARCH_CHILDREN);
    av_opt_set(srcFilterContext, "sample_fmt", av_get_sample_fmt_name(ad->pCodecContext->sample_fmt),
               AV_OPT_SEARCH_CHILDREN);
    av_opt_set_q(srcFilterContext, "time_base", (AVRational) {1, ad->pCodecContext->sample_rate},
                 AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(srcFilterContext, "sample_rate", ad->pCodecContext->sample_rate, AV_OPT_SEARCH_CHILDREN);
    int resp = avfilter_init_str(srcFilterContext, nullptr);

    if (resp < 0) {
        cout << "ERROR: creating srcFilter: " << av_err2str(resp) << endl;
        return resp;
    }
    cout << "created srcFilter!" << endl;
    return 0;
}

int AudioFilter::initSinkFilter() {

    int resp = 0;

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
//create sink filter - used for the output of the data
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
    /* A different way of passing the options is as key/value pairs in a
     * dictionary. */
    int resp = 0;
    AVDictionary *optionsDict = nullptr;
    av_dict_set(&optionsDict, "volume", AV_STRINGIFY(VOLUME_VAL), 0);
    resp = avfilter_init_dict(volumeFilterContext, &optionsDict);
    av_dict_free(&optionsDict);

    if (resp < 0) {
        cout << "Could not initialize the volume filter: " << av_err2str(resp) << endl;
        return resp;
    }
    cout << "created volume Filter!" << endl;
    return 0;
}
