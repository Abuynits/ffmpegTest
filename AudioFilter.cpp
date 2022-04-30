//
// Created by Alexiy Buynitsky on 4/29/22.
//

#include "AudioFilter.h"

int AudioFilter::initializeAllObjets(AudioDecoder ad, int audio_stream_index) {
    //filtergraph houses all of our filters that we will use
    filterGraph = avfilter_graph_alloc();
    if (filterGraph == nullptr) {
        cout << "ERROR: Unable to create filterGraph" << endl;
        return AVERROR(ENOMEM);
    }
    //create aBuffer filter, used for inputing data to filtergraph
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

    inputs = avfilter_inout_alloc();
    outputs = avfilter_inout_alloc();

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
                                            args, NULL, filterGraph);
    if (resp < 0) {
        cout << "ERROR: creating srcFilter: " << av_err2str(resp) << endl;
        return resp;
    }


}

AudioFilter::AudioFilter() {

}
