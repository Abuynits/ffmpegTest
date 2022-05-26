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
//    resampleCtx = swr_alloc_set_opts(nullptr,
//                                     av_get_default_channel_layout(ad->pOutCodecContext->channels),
//                                     ad->pOutCodecContext->sample_fmt,
//                                     ad->pOutCodecContext->sample_rate,
//                                     av_get_default_channel_layout(ad->pInCodecContext->channels),
//                                     ad->pInCodecContext->sample_fmt,
//                                     ad->pInCodecContext->sample_rate,
//                                     0, nullptr);

    //set channels for conversions:
    av_opt_set_sample_fmt(resampleCtx, "in_sample_fmt", ad->pInCodecContext->sample_fmt, 0);
    av_opt_set_sample_fmt(resampleCtx, "out_sample_fmt", ad->pOutCodecContext->sample_fmt, 0);

    av_opt_set_int(resampleCtx, "in_sample_rate", ad->pInCodecContext->sample_rate, 0);
    av_opt_set_int(resampleCtx, "out_sample_rate", ad->pOutCodecContext->sample_rate, 0);

    av_opt_set_channel_layout(resampleCtx, "in_channel_layout", ad->pInCodecContext->channel_layout, 0);
    av_opt_set_channel_layout(resampleCtx, "out_channel_layout", ad->pOutCodecContext->channel_layout, 0);
//    channel count and layout are not set

//    av_assert0(ad->pOutCodecContext->sample_rate == ad->pInCodecContext->sample_rate);
    //init sample context:
    resp = swr_init(resampleCtx);
    if (resp != 0) {
        cerr << "ERROR: Could not init context" << endl;
        return -1;
    }
    //TODO: continue to try to make resampling work through resampling context
    //use the following guide;https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/resampling_audio.c
    srcNumChannels = ad->pInCodecContext->channels;
    resp = av_samples_alloc_array_and_samples(&srcData, &srcLineSize, srcNumChannels,srcNumSamples, ad->pInCodecContext->sample_fmt,0);
    if(resp<0){
        cout<<"error: could not allocate source samples:"<<endl;
        exit(1);
    }
    maxDstNumSamples = dstNumSamples = av_rescale_rnd(srcNumSamples, ad->pInCodecContext->sample_rate, ad->pOutCodecContext->sample_rate, AV_ROUND_UP);

    dstNumChannels  =  ad->pOutCodecContext->channels;
    resp = av_samples_alloc_array_and_samples(&dstData, &dstLineSize, dstNumChannels,dstNumSamples, ad->pInCodecContext->sample_fmt,0);
    if(resp<0) {
        cout << "error: could not allocate destination samples:" << endl;
        exit(1);
    }



    return 0;

}