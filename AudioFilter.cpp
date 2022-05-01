//
// Created by Alexiy Buynitsky on 4/29/22.
//

#include "AudioFilter.h"

int AudioFilter::initializeAllObjets(AudioDecoder ad, char *filterDescription, int audio_stream_index) {
    //filtergraph houses all of our filters that we will use
    filterGraph = avfilter_graph_alloc();
    if (filterGraph == nullptr) {
        cout << "ERROR: Unable to create filterGraph" << endl;
        return AVERROR(ENOMEM);
    }
    //create aBuffer filter, used for inputing data to filtergraph -> recieves frames from the decoder
    srcFilter = avfilter_get_by_name("abuffer");
    if (srcFilter == nullptr) {
        cout << "ERROR: Could not find the abuffer filter" << endl;

        return AVERROR_FILTER_NOT_FOUND;
    }
    srcFilterContext = avfilter_graph_alloc_filter(filterGraph, srcFilter, "src");
    if (srcFilterContext == nullptr) {
        cout << "Could not allocate the inputFiler instance" << endl;
        return AVERROR(ENOMEM);
    }


    //check if have channel layout:
    if (!ad.pCodecContext->channel_layout) {
        cout << "warning: channel context not initialized... initializing" << endl;
        ad.pCodecContext->channel_layout = av_get_default_channel_layout(ad.pCodecContext->channels);
    }

    AVRational time_base = ad.pFormatContext->streams[audio_stream_index]->time_base;
    //store the value in args as if you were going to print.
    snprintf(args, sizeof(args),
             "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%"
    PRIx64,
            time_base.num, time_base.den, ad.pCodecContext->sample_rate,
            av_get_sample_fmt_name(ad.pCodecContext->sample_fmt), ad.pCodecContext->channel_layout);
    int resp = avfilter_graph_create_filter(&srcFilterContext, srcFilter, "in",
                                            args, nullptr, filterGraph);

    if (resp < 0) {
        cout << "ERROR: creating srcFilter: " << av_err2str(resp) << endl;
        return resp;
    }

    //create sink filter - used for the output of the data
    resp = avfilter_graph_create_filter(&srcFilterContext, srcFilter, "out",
                                        args, nullptr, filterGraph);

    if (resp < 0) {
        cout << "ERROR: creating sinkFilter: " << av_err2str(resp) << endl;
        return resp;
    }

    resp = av_opt_set_int_list(sinkFilter, "sample_fmts", ad.pCodec->sample_fmts, -1,
                               AV_OPT_SEARCH_CHILDREN);
    if (resp < 0) {
        cout << "ERROR: output format cannot be set: " << av_err2str(resp) << endl;
        return resp;
    }

    resp = av_opt_set_int_list(sinkFilterContext, "channel_layouts", ad.pCodec->channel_layouts, -1,
                               AV_OPT_SEARCH_CHILDREN);
    if (resp < 0) {
        cout << "ERROR: channel layout cannot be set: " << av_err2str(resp) << endl;
        return resp;
    }
    //NOTE: using suported_samplerates
    resp = av_opt_set_int_list(sinkFilterContext, "sample_rates", ad.pCodec->supported_samplerates, -1,
                               AV_OPT_SEARCH_CHILDREN);

    //set the endpoints for the inputs and outputs of the filter
    inputs = avfilter_inout_alloc();
    outputs = avfilter_inout_alloc();

    outputs->name = av_strdup("in");
    outputs->filter_ctx = srcFilterContext;
    outputs->pad_idx = 0;
    outputs->next = nullptr;
    inputs->name = av_strdup("out");
    inputs->filter_ctx = sinkFilterContext;
    inputs->pad_idx = 0;
    inputs->next = nullptr;
    resp = avfilter_graph_parse_ptr(filterGraph, filterDescription,
                                    &inputs, &outputs, nullptr);
    if (resp < 0) {
        cout << "ERROR: cannot add graph described by input " << av_err2str(resp) << endl;
        return resp;
    }
    resp = avfilter_graph_config(filterGraph, nullptr);
    if (resp < 0) {
        cout << "ERROR: cannot configure filter graph " << av_err2str(resp) << endl;
        return resp;
    }
    outlink = sinkFilterContext->inputs[0];
    av_get_channel_layout_string(args, sizeof(args), -1, outlink->channel_layout);

    av_log(NULL, AV_LOG_INFO, "Output: srate:%dHz fmt:%s chlayout:%s\n",
           (int) outlink->sample_rate,
           (char *) av_x_if_null(av_get_sample_fmt_name(static_cast<AVSampleFormat>(outlink->format)), "?"),
           args);
    return 0;
}

AudioFilter::AudioFilter() {

}
