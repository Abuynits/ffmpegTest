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
    //create aBuffer filter, used for inputing data to filtergraph
    aFilter = avfilter_get_by_name("abuffer");
    if (aFilter == nullptr) {
        cout << "ERROR: Could not find the abuffer filter" << endl;

        return AVERROR_FILTER_NOT_FOUND;
    }
    aFilterContext = avfilter_graph_alloc_filter(filterGraph, aFilter, "src");
    if (aFilter == nullptr) {
        cout << "Could not allocate the inputFiler instance" << endl;
        return AVERROR(ENOMEM);
    }
    /* Set the filter options through the AVOptions API. */
    av_channel_layout_describe(&INPUT_CHANNEL_LAYOUT, ch_layout, sizeof(ch_layout));
    av_opt_set(aFilterContext, "channel_layout", ch_layout, AV_OPT_SEARCH_CHILDREN);
    av_opt_set(aFilterContext, "sample_fmt", av_get_sample_fmt_name(INPUT_FORMAT), AV_OPT_SEARCH_CHILDREN);
    av_opt_set_q(aFilterContext, "time_base", (AVRational) {1, INPUT_SAMPLERATE}, AV_OPT_SEARCH_CHILDREN);
    /* Now initialize the filter; we pass NULL options, since we have already
 * set all the options above. */
    err = avfilter_init_str(aFilterContext, NULL);
    if (err < 0) {
        fprintf(stderr, "Could not initialize the abuffer filter.\n");
        return err;
    }

}
