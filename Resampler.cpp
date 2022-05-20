//
// Created by Alexiy Buynitsky on 5/20/22.
//

#include "Resampler.h"

int Resampler::initObjects() {
    resampleCtx = swr_alloc();
    if (resampleCtx == nullptr) {
        cerr << "ERROR: Could not allocate resample context" << endl;
        return -1;
    }
    //set channels for conversions:
    av_opt_set_sample_fmt(resampleCtx, "in_sample_fmt", ad->pCodecContext->sample_fmt, 0);
    av_opt_set_sample_fmt(resampleCtx, "out_sample_fmt", ad->pCodecContext->sample_fmt, 0);

    av_opt_set_int(resampleCtx, "in_sample_rate", ad->pCodecContext->sample_rate, 0);
    av_opt_set_int(resampleCtx, "out_sample_rate", ad->pCodecContext->sample_rate, 0);

    av_opt_set_channel_layout(resampleCtx, "in_chlayout", ad->pCodecContext->channel_layout, 0);
    av_opt_set_channel_layout(resampleCtx, "out_chlayout", ad->pCodecContext->channel_layout, 0);

}