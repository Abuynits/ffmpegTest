//
// Created by Alexiy Buynitsky on 5/20/22.
//

#include "Resampler.h"

int Resampler::initObjects() {
    int resp;
    resampleCtx = swr_alloc();
    if (resampleCtx == nullptr) {
        cerr << "ERROR: Could not allocate resample context" << endl;
        return -1;
    }
    //set channels for conversions:
    //TODO: potential probelm as set all of this from 1 parameter which remains the same
    av_opt_set_sample_fmt(resampleCtx, "in_sample_fmt", ad->pCodecContext->sample_fmt, 0);
    av_opt_set_sample_fmt(resampleCtx, "out_sample_fmt", ad->pCodecContext->sample_fmt, 0);

    av_opt_set_int(resampleCtx, "in_sample_rate", ad->pCodecContext->sample_rate, 0);
    av_opt_set_int(resampleCtx, "out_sample_rate", ad->pCodecContext->sample_rate, 0);

    av_opt_set_channel_layout(resampleCtx, "in_chlayout", ad->pCodecContext->channel_layout, 0);
    av_opt_set_channel_layout(resampleCtx, "out_chlayout", ad->pCodecContext->channel_layout, 0);

    //init sample context:
    resp = swr_init(resampleCtx);
    if (resp != 0) {
        cerr << "ERROR: Could not init context" << endl;
        return -1;
    }
    numSrcChannels = av_get_channel_layout_nb_channels(ad->pCodecContext->channel_layout);

    numSrcSamples = ad->pCodecContext->sample_rate;//TODO: chekc whether this is the right parameter
    srcLineSize = 8; //TODO:NOTE: line size is from ad->pFrame->linesize (default is 8)

    resp = av_samples_alloc_array_and_samples(&srcData, &srcLineSize, numSrcChannels,
                                              numSrcSamples, ad->pCodecContext->sample_fmt, 0);
    if (resp < 0) {
        cerr << "ERROR: Could allocate contexts" << endl;
        return -1;
    }
    maxDstNumSamples = numDstSamples = av_rescale_rnd(numSrcSamples, ad->pCodecContext->sample_rate,
                                                      ad->pCodecContext->sample_rate, AV_ROUND_UP);
    numDstChannels=ad->pCodecContext->channel_layout.nb_channels
    cout << "allocated resampling context!" << endl;
    return 0;

}