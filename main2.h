////
//// Created by Alexiy Buynitsky on 4/23/22.
////
//extern "C" {
//#include <libavutil/frame.h>
//#include <libavutil/mem.h>
//#include <libavformat/avformat.h>
//#include <libavcodec/avcodec.h>
//#include <libavutil/timestamp.h>
//#include <libavutil/samplefmt.h>
//#include <libavutil/opt.h>
//#include <libavutil/channel_layout.h>
//#include <libavutil/samplefmt.h>
//#include <libswresample/swresample.h>
//}
//
//#include "AudioDecoder.h"
//#include "AudioFilter.h"
//#include "OutputAnalysis.h"
//#include "Resampler.h"
//#include <iostream>
//#include <cstdio>
//#include <cstdlib>
//#include <cstring>
//
///**
// * first loop: loops over the frames present in a packet
// * runs them through the filters and handles eof/ not enough frames
// * @param showFrameData boolean whether to display the info of each frame during output
// * @return whether an error occured
// */
//int loopOverPacketFrames(bool showFrameData);
//
///**
// * adds the frames to the head node of filtergraph
// * recieves the output from the filtergraph
// * writes the raw data to the output file
// * @return whether an error occured
// */
//int filterAudioFrame();
//
///**
// * transfers parameters from input codec to output codec
// * estimates the raw data based on the bitrate and other parameters
// * writes the correct header and tail to the final output path
// * transfers the raw data to the output data
// * @param inputPts  tracks the points being looped over
// * @return whether an error occured
// */
//int transferStreamData(int *inputPts);
//
///**
// * first loop: runs all of the frames through the filters
// * writes a header and tail, but they will be fixed in the second loop
// * @return whether an error occured
// */
//int applyFilters(bool writeMetaData);
//
///**
// * the second loop: fixes the headers and tails of the first loop
// * transfers the data as is, but creates custon headers and tails
// * @return whether an error occured
// */
//int applyMuxers();
//
///**
// * displays the audio information
// * this includes the start and end frames, the total frame
// * the before and after RMS of the trough and peak of a silent frame
// */
//void getAudioInfo();
//
//int resampleAudio(bool showFrameData);
//
//int processAudioStack(int *pInt);
//
//static int read_decode_convert_and_store(int *finished);
//
//using namespace std;
////info about codecs, and is responsible for processing audio
//AudioDecoder *ad;
////list of filters and how they are linked to each other
//AudioFilter *av;
////audio info about the frames and the rms
//OutputAnalysis *audioInfo;
////The resampler that converts any input file to a wav for looping and processing
//Resampler *rs;
////the input file path
//
////TODO: have an error with writing the file headers:
////TODO: not work when given anything but a wav input
//const char *inputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/inputRecordings/recording.wav";
//
////const char *wavInputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/inputRecordings/wavInput.wav";
////stores the raw data after applying filters
//const char *tempFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/tempRecording.wav";
////the output file path
//const char *finalFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/outputRecordings/recordingOut2.wav";
////stores the stderr output which contains rms stats and other debug info
//const char *statOutFP = "/Users/abuynits/CLionProjects/ffmpegTest5/output.txt";
///*
// * file path process:
// * convert .___ to .wav: inputFP -> wavInputFP
// * strip wavInputFP and get raw wav data, store it in temp FP: wavInput->tempFP
// * write the correct headers: tempFP -> finalFP
// */
//
//int totalFrameCount = 0;
//const bool showData = false;
//const bool writeFileHeader = true;
//
//
//int main() {
//    //used to error return errors
//    int resp;
//    //create audioInfo
//    audioInfo = new OutputAnalysis(statOutFP, false);
//    //create audio decoder
//    ad = new AudioDecoder(inputFP, tempFP, true, true);
//    ad->openFiles();
//    ad->initializeAllObjects();
//
//    if (resp < 0) {
//        cerr << "error: could not initialize decoder" << endl;
//        return 1;
//    }
//    cerr << "initialized AudioDecoder" << endl;
//    //create audio filter
//    av = new AudioFilter(ad);
//    resp = av->initializeAllObjets();
//    if (resp < 0) {
//        cerr << "error: could not initialize filters" << endl;
//        return 1;
//    }
//    cerr << "initialized AudioFilter\n" << endl;
//    rs = new Resampler(ad);
//    rs->initObjects();
//    cerr << "initialized Resampler\n" << endl;
//    resp = resampleAudio(showData);
//    if (resp == 0) {
//        cerr << "finished processing first loop" << endl;
//    }
//    return 0;
//}
//
//int resampleAudio(bool showFrameData) {
//    int ret;
//    while (1) {
//        /** Use the encoder's desired frame size for processing. */
//        const int output_frame_size = ad->pOutCodecContext->frame_size;
//        int finished = 0;
//
//        /**
//   * Make sure that there is one frame worth of samples in the FIFO
//      * buffer so that the encoder can do its work.
//      * Since the decoder's and the encoder's frame size may differ, we
//      * need to FIFO buffer to store as many frames worth of input samples
//      * that they make up at least one frame worth of output samples.
//      */
//        while (av_audio_fifo_size(ad->avBuffer) < output_frame_size) {
//            /**
//   * Decode one frame worth of audio samples, convert it to the
//   * output sample format and put it into the FIFO buffer.
//   */
//            if (read_decode_convert_and_store(&finished))
//                goto cleanup;
//
//            /**
//   * If we are at the end of the input file, we continue
//   * encoding the remaining audio samples to the output file.
//   */
//            if (finished)
//                break;
//        }
//
//        /**
//              * If we have enough samples for the encoder, we encode them.
//              * At the end of the file, we pass the remaining samples to
//              * the encoder.
//              */
//        while (av_audio_fifo_size(ad->avBuffer) >= output_frame_size ||
//               (finished && av_audio_fifo_size(ad->avBuffer) > 0))
//            /**
//   * Take one frame worth of audio samples from the FIFO buffer,
//   * encode it and write it to the output file.
//   */
//            if (load_encode_and_write(ad->avBuffer, ad->pOutFormatContext,
//                                      ad->pOutCodecContext))
//                goto cleanup;
//
//        /**
// * If we are at the end of the input file and have encoded
// * all remaining samples, we can exit this loop and finish.
// */
//        if (finished) {
//            int data_written;
//            /** Flush the encoder as it may have delayed frames. */
//            do {
//                if (encode_audio_frame(NULL, ad->pOutFormatContext,
//                                       ad->pOutCodecContext, &data_written))
//                    goto cleanup;
//            } while (data_written);
//            break;
//        }
//    }
//
//    /** Write the trailer of the output file container. */
//    av_write_trailer(ad->pOutFormatContext);
//    ret = 0;
//
//    cleanup:
//    if (ad->avBuffer)
//        av_audio_fifo_free(ad->avBuffer);
//    swr_free(&rs->resampleCtx);
//    if (ad->pOutCodecContext)
//        avcodec_close(ad->pOutCodecContext);
//    if (ad->pOutFormatContext) {
//        avio_close(ad->pOutFormatContext->pb);
//        avformat_free_context(ad->pOutFormatContext);
//    }
//    if (ad->pInCodecContext)
//        avcodec_close(ad->pInCodecContext);
//    if (ad->pInFormatContext)
//        avformat_close_input(&ad->pInFormatContext);
//    return -1;
//}
//
//static int read_decode_convert_and_store(int *finished) {
//
//    /** Temporary storage for the converted input samples. */
//    uint8_t **converted_input_samples = NULL;
//    int data_present;
//    int ret = AVERROR_EXIT;
//
//    /** Decode one frame worth of audio samples. */
//    if (decode_audio_frame(ad->pInFrame, ad->pInFormatContext,
//                           ad->pInCodecContext, &data_present, finished))
//        goto cleanup;
//    /**
//       * If we are at the end of the file and there are no more samples
//       * in the decoder which are delayed, we are actually finished.
//       * This must not be treated as an error.
//       */
//    if (*finished && !data_present) {
//        ret = 0;
//        goto cleanup;
//    }
//    /** If there is decoded data, convert and store it */
//    if (data_present) {
//        /** Initialize the temporary storage for the converted input samples. */
//        if (init_converted_samples(&converted_input_samples, ad->pOutCodecContext,
//                                   ad->pInFrame->nb_samples))
//            goto cleanup;
//
//        /**
//           * Convert the input samples to the desired output sample format.
//           * This requires a temporary storage provided by converted_input_samples.
//           */
//        if (convert_samples((const uint8_t **) ad->pInFrame->extended_data, converted_input_samples,
//                            ad->pInFrame->nb_samples, rs->resampleCtx))
//            goto cleanup;
//
//        /** Add the converted input samples to the FIFO buffer for later processing. */
//        if (add_samples_to_fifo(ad->avBuffer, converted_input_samples,
//                                ad->pInFrame->nb_samples))
//            goto cleanup;
//        ret = 0;
//    }
//    ret = 0;
//
//    cleanup:
//    if (converted_input_samples) {
//        av_freep(&converted_input_samples[0]);
//        free(converted_input_samples);
//    }
//    av_frame_free(&ad->pInFrame);
//
//    return ret;
//}