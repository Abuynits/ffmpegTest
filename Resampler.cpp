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
<<<<<<< HEAD
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
=======
    //set channels for conversions:
    //TODO: potential probelm as set all of this from 1 parameter which remains the same
    av_opt_set_sample_fmt(resampleCtx, "in_sample_fmt", ad->pCodecContext->sample_fmt, 0);
    av_opt_set_sample_fmt(resampleCtx, "out_sample_fmt", ad->pCodecContext->sample_fmt, 0);

    av_opt_set_int(resampleCtx, "in_sample_rate", ad->pCodecContext->sample_rate, 0);
    av_opt_set_int(resampleCtx, "out_sample_rate", ad->pCodecContext->sample_rate, 0);

    av_opt_set_channel_layout(resampleCtx, "in_chlayout", ad->pCodecContext->channel_layout, 0);
    av_opt_set_channel_layout(resampleCtx, "out_chlayout", ad->pCodecContext->channel_layout, 0);

>>>>>>> ea6eb83e5879537ab8d025fbb8bf0e3b79bd9d61
    //init sample context:
    resp = swr_init(resampleCtx);
    if (resp != 0) {
        cerr << "ERROR: Could not init context" << endl;
        return -1;
    }
<<<<<<< HEAD
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



=======
    numSrcChannels = av_get_channel_layout_nb_channels(ad->pCodecContext->channel_layout);

    numSrcSamples = ad->pCodecContext->sample_rate;//TODO: chekc whether this is the right parameter
    srcLineSize = 8; //TODO:NOTE: line size is from ad->pFrame->linesize (default is 8)

    resp = av_samples_alloc_array_and_samples(&srcData, &srcLineSize, numSrcChannels,
                                              numSrcSamples, ad->pCodecContext->sample_fmt, 0);
    if (resp < 0) {
        cerr << "ERROR: Could not allocate source sample" << endl;
        return -1;
    }
    maxDstNumSamples = numDstSamples = av_rescale_rnd(numSrcSamples, ad->pCodecContext->sample_rate,
                                                      ad->pCodecContext->sample_rate, AV_ROUND_UP);
    numDstChannels=ad->pCodecContext->channels;//TODO: potential bug here: need to probably change dst layout

    dstLineSize=srcLineSize;//TODO: need to check to line size

    numDstChannels = av_get_channel_layout_nb_channels(ad->pCodecContext->channel_layout);//TODO: need to potentially change

    resp = av_samples_alloc_array_and_samples(&dstData, &dstLineSize, numDstChannels,
                                              numDstSamples, ad->pCodecContext->sample_fmt, 0);
    if (resp < 0) {
        cerr << "ERROR: Could not allocate destination samplel" << endl;
        return -1;
    }

    cout << "allocated all sampling contexts context!" << endl;
>>>>>>> ea6eb83e5879537ab8d025fbb8bf0e3b79bd9d61
    return 0;

}