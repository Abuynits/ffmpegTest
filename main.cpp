//
// Created by Alexiy Buynitsky on 4/23/22.
//
extern "C" {
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/timestamp.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

#include "AudioDecoder.h"
#include "AudioFilter.h"
#include "OutputAnalysis.h"
#include "Resampler.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

/**
 * first loop: loops over the frames present in a packet
 * runs them through the filters and handles eof/ not enough frames
 * @param showFrameData boolean whether to display the info of each frame during output
 * @return whether an error occured
 */
int loopOverPacketFrames(bool showFrameData);

/**
 * adds the frames to the head node of filtergraph
 * recieves the output from the filtergraph
 * writes the raw data to the output file
 * @return whether an error occured
 */
int filterAudioFrame();

/**
 * transfers parameters from input codec to output codec
 * estimates the raw data based on the bitrate and other parameters
 * writes the correct header and tail to the final output path
 * transfers the raw data to the output data
 * @param inputPts  tracks the points being looped over
 * @return whether an error occured
 */
int transferStreamData(int *inputPts);

/**
 * first loop: runs all of the frames through the filters
 * writes a header and tail, but they will be fixed in the second loop
 * @return whether an error occured
 */
int applyFilters(bool writeMetaData);

/**
 * the second loop: fixes the headers and tails of the first loop
 * transfers the data as is, but creates custon headers and tails
 * @return whether an error occured
 */
int applyMuxers();

/**
 * displays the audio information
 * this includes the start and end frames, the total frame
 * the before and after RMS of the trough and peak of a silent frame
 */
void getAudioInfo();

int resampleAudio(bool showFrameData);

int read_decode_convert_and_store(int *finished);

int decode_audio_frame(int *dataPresent, int *finished);

int transformAudioFrame(bool showFrameData);

using namespace std;
//info about codecs, and is responsible for processing audio
AudioDecoder *ad;
//list of filters and how they are linked to each other
AudioFilter *av;
//audio info about the frames and the rms
OutputAnalysis *audioInfo;
//The resampler that converts any input file to a wav for looping and processing
Resampler *rs;
//the input file path

//TODO: have an error with writing the file headers:
//TODO: not work when given anything but a wav input
const char *inputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/inputRecordings/recording.aac";

//const char *wavInputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/inputRecordings/wavInput.wav";
//stores the raw data after applying filters
const char *tempFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/tempRecording.wav";
//the output file path
const char *finalFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/outputRecordings/recordingOut2.wav";
//stores the stderr output which contains rms stats and other debug info
const char *statOutFP = "/Users/abuynits/CLionProjects/ffmpegTest5/output.txt";
/*
 * file path process:
 * convert .___ to .wav: inputFP -> wavInputFP
 * strip wavInputFP and get raw wav data, store it in temp FP: wavInput->tempFP
 * write the correct headers: tempFP -> finalFP
 */

int totalFrameCount = 0;
const bool showData = false;
const bool writeFileHeader = true;


int main() {
    //used to error return errors
    int resp;
    //create audioInfo
    audioInfo = new OutputAnalysis(statOutFP, false);
    //create audio decoder
    ad = new AudioDecoder(inputFP, tempFP, true, true);
    ad->openFiles();
    ad->initializeAllObjects();

    if (resp < 0) {
        cerr << "error: could not initialize decoder" << endl;
        return 1;
    }
    cerr << "initialized AudioDecoder" << endl;
    //create audio filter
    av = new AudioFilter(ad);
    resp = av->initializeAllObjets();
    if (resp < 0) {
        cerr << "error: could not initialize filters" << endl;
        return 1;
    }
    cerr << "initialized AudioFilter\n" << endl;
    rs = new Resampler(ad);
    rs->initObjects();
//    cerr << "initialized Resampler\n" << endl;
//    resp = resampleAudio(showData);
//    if (resp == 0) {
//        cerr << "finished processing first loop" << endl;
//    }
    // TODO: this is part of the second loop: need to add another initialization
//    loop over the frames in each packet and apply a chain of filters to each frame
    resp = transformAudioFrame(showData);
    if (resp == 0) {
        cerr << "finished processing first loop" << endl;
    }

//    resp = applyFilters(writeFileHeader);
//    if (resp == 0) {
//        cerr << "finished processing first loop" << endl;
//    }

    //close all of the objects
    ad->closeAllObjects();
    av->closeAllObjects();
    //save the stats from the filter stage into the info object
    audioInfo->setFrameVals(ad->startFrame, ad->endFrame, totalFrameCount);

    cout << "=====================DONE WITH SECOND LOOP=====================" << endl;
    //========================SECOND STAGE: make playable by wav output file==========================
//    ad = new AudioDecoder(tempFP, finalFP, false, true);
//    ad->openFiles();
//    ad->initializeAllObjects();
//
//    //loop over the file to get the correct header to be playable
//    resp = applyMuxers();
//    if (resp == 0) {
//        cerr << "finished muxing files" << endl;
//    }
//
//    ad->closeAllObjects();
////    //==============RMS PROCESSING===================
////get the stats and show them
//    getAudioInfo();
//
//    cout << "=====================DONE WITH THIRD LOOP=====================" << endl;
    return 0;
}

int transferStreamData(int *inputPts) {
    AVStream *inStream, *outStream;
    inStream = ad->pInFormatContext->streams[ad->pPacket->stream_index];

    if (ad->pPacket->stream_index >= ad->streamMappingSize ||
        ad->streamMapping[ad->pPacket->stream_index] < 0) {
        av_packet_unref(ad->pPacket);
        return 1;
    }

    ad->pPacket->stream_index = ad->streamMapping[ad->pPacket->stream_index];

    outStream = ad->pOutFormatContext->streams[ad->pPacket->stream_index];

    ad->pPacket->duration = av_rescale_q(ad->pPacket->duration, inStream->time_base, outStream->time_base);

    ad->pPacket->pts = *inputPts;
    ad->pPacket->dts = *inputPts;
    *inputPts += ad->pPacket->duration;
    ad->pPacket->pos = -1;
    return 0;
}

int loopOverPacketFrames(bool showFrameData) {
    int resp = avcodec_send_packet(ad->pInCodecContext, ad->pPacket);
    if (resp < 0) {
        cerr << "error submitting a packet for decoding: " << av_err2str(resp);
        return resp;
    }

    while (resp >= 0) {
        resp = avcodec_receive_frame(ad->pInCodecContext, ad->pInFrame);
        if (resp == AVERROR(EAGAIN)) {
            if (showFrameData) cerr << "Not enough data in frame, skipping to next packet" << endl;
            //decoded not have enough data to process frame
            //not error unless reached end of the stream - pass more packets untill have enough to produce frame
            clearFrames:
            av_frame_unref(ad->pInFrame);
            av_freep(ad->pInFrame);
            break;
        } else if (resp == AVERROR_EOF) {
            cerr << "Reached end of file" << endl;
            goto clearFrames;
        } else if (resp < 0) {
            cerr << "Error while receiving a frame from the decoder: " << av_err2str(resp) << endl;
            // Failed to get a frame from the decoder
            av_frame_unref(ad->pInFrame);
            av_freep(ad->pInFrame);
            return resp;
        }
        /*
         * TODO: need to find the noise level of audio file
         * try to look at astats filter, then at the portions where silence is detected idk
         * need to get the RMS factor: what kolya talk about
         */

        if (showFrameData)
            cerr << "frame number: " << ad->pInCodecContext->frame_number
                 << ", Pkt_Size: " << ad->pInFrame->pkt_size
                 << ", Pkt_pts: " << ad->pInFrame->pts
                 << ", Pkt_keyFrame: " << ad->pInFrame->key_frame << endl;

        if (filterAudioFrame() < 0) {
            cerr << "error in filtering" << endl;

        }

        av_frame_unref(ad->pInFrame);
        av_freep(ad->pInFrame);
        if (resp < 0) {
            return resp;
        }
        totalFrameCount++;
    }
    return 0;

}

int filterAudioFrame() {
    //add to source frame:
    int resp = av_buffersrc_add_frame(av->srcContext, ad->pInFrame);
    //TODO:START
    if (resp < 0) {
        cerr << "Error: cannot send to graph: " << av_err2str(resp) << endl;
        breakFilter:
        av_frame_unref(ad->pInFrame);
        return resp;
    }
    //get back the filtered data:
    while ((resp = av_buffersink_get_frame(av->sinkContext, ad->pInFrame)) >= 0) {

        if (resp < 0) {
            cerr << "Error filtering data " << av_err2str(resp) << endl;
            goto breakFilter;
        }

        ad->saveAudioFrame(showData);

        if (resp < 0) {
            cerr << "Error muxing packet" << endl;
        }
        av_frame_unref(ad->pInFrame);
    }
    return 0;
}

int applyFilters(bool writeMetaData) {
    int resp = 0;
    if (writeMetaData) {
        resp = avformat_write_header(ad->pOutFormatContext, nullptr);
        if (resp < 0) {
            cerr << "Error when writing header" << endl;
            return -1;
        }

    }

    while (av_read_frame(ad->pInFormatContext, ad->pPacket) >= 0) {

        resp = loopOverPacketFrames(showData);
        if (resp < 0) {
            break;
        }
    }
    cout << "here" << endl;
    //flush the audio decoder
    ad->pPacket = nullptr;
    loopOverPacketFrames(showData);
    if (writeMetaData) {
        av_write_trailer(ad->pOutFormatContext);
    }

    resp = ad->getAudioRunCommand();
    if (resp < 0) {
        cerr << "ERROR getting ffplay command" << endl;
        return resp;
    }

    return 0;
}

int applyMuxers() {
    int resp = avformat_write_header(ad->pOutFormatContext, nullptr);
    if (resp < 0) {
        cerr << "Error when opening output file" << endl;
        return -1;
    }
    int inputPts = 0;
    while (1) {
        resp = av_read_frame(ad->pInFormatContext, ad->pPacket);
        if (resp < 0) {
            break;
        }

        if (transferStreamData(&inputPts) == 1) { continue; }

        resp = av_interleaved_write_frame(ad->pOutFormatContext, ad->pPacket);
        if (resp < 0) {
            cerr << "Error muxing packet" << endl;
            break;
        }
        av_packet_unref(ad->pPacket);
    }
    av_write_trailer(ad->pOutFormatContext);
    return 0;
}

void getAudioInfo() {
    cerr.flush();
    audioInfo->getRMS();

    cout << "===============VIDEO DATA===============" << endl;
    cout << "start frame " << audioInfo->startFrame << " to " << audioInfo->endFrame << " of " << audioInfo->totalFrame
         << " frames"
         << endl;
    cout << "before trough rms: " << audioInfo->bTrough << " DB after trough rms: " << audioInfo->aTrough << " DB"
         << endl;
    cout << "before peak rms: " << audioInfo->bPeak << " DB after peak rms: " << audioInfo->aPeak << " DB" << endl;
}

int transformAudioFrame(bool showFrameData) {
    int resp = 0;
    resp = avformat_write_header(ad->pOutFormatContext, nullptr);
    if (resp < 0) {
        cerr << "Error when writing header" << endl;
        return -1;
    }
    while (av_read_frame(ad->pInFormatContext, ad->pPacket) >= 0) {
        while (resp >= 0) {
            resp = avcodec_receive_frame(ad->pInCodecContext, ad->pInFrame);
            if (resp == AVERROR(EAGAIN)) {
                if (showFrameData) cerr << "Not enough data in frame, skipping to next packet" << endl;
                //decoded not have enough data to process frame
                //not error unless reached end of the stream - pass more packets untill have enough to produce frame
                clearFrames:
                av_frame_unref(ad->pInFrame);
                av_freep(ad->pInFrame);
                break;
            } else if (resp == AVERROR_EOF) {
                cerr << "Reached end of file" << endl;
                goto clearFrames;
            } else if (resp < 0) {
                cerr << "Error while receiving a frame from the decoder: " << av_err2str(resp) << endl;
                // Failed to get a frame from the decoder
                av_frame_unref(ad->pInFrame);
                av_freep(ad->pInFrame);
                return resp;
            }
            /*
             * TODO: need to find the noise level of audio file
             * try to look at astats filter, then at the portions where silence is detected idk
             * need to get the RMS factor: what kolya talk about
             */

            if (showFrameData)
                cerr << "frame number: " << ad->pInCodecContext->frame_number
                     << ", Pkt_Size: " << ad->pInFrame->pkt_size
                     << ", Pkt_pts: " << ad->pInFrame->pts
                     << ", Pkt_keyFrame: " << ad->pInFrame->key_frame << endl;

            //TODO: add the rescale of samples and conversion and writing.
            rs->dstNumSamples = av_rescale_rnd(swr_get_delay(rs->resampleCtx, ad->pInCodecContext->sample_rate) +
                                               rs->srcNumSamples, ad->pOutCodecContext->sample_rate,
                                               ad->pInCodecContext->sample_rate, AV_ROUND_UP);

            if (rs->dstNumSamples > rs->maxDstNumSamples) {
                av_freep(&(rs->dstData[0]));
                resp = av_samples_alloc(rs->dstData, &rs->dstLineSize, rs->dstNumChannels,
                                        rs->dstNumSamples, ad->pOutCodecContext->sample_fmt, 1);
                if (resp < 0)
                    break;
                rs->maxDstNumSamples = rs->dstNumSamples;
            }
            /* convert to destination format */
            resp = swr_convert(rs->resampleCtx, rs->dstData, rs->dstNumSamples, (const uint8_t **) rs->srcData,
                               rs->srcNumSamples);
            if (resp < 0) {
                fprintf(stderr, "Error while converting\n");
                goto end;
            }
            rs->dstBufferSize = av_samples_get_buffer_size(&rs->dstLineSize, rs->dstNumChannels,
                                                           resp, ad->pOutCodecContext->sample_fmt, 1);
            if (rs->dstBufferSize < 0) {
                fprintf(stderr, "Could not get sample buffer size\n");
                goto end;
            }
            printf("in:%d out:%d\n", rs->srcNumSamples, resp);
            fwrite(rs->dstData[0], 1, rs->dstBufferSize, ad->outFile);
            av_frame_unref(ad->pInFrame);
            av_freep(ad->pInFrame);
            if (resp < 0) {
                return resp;
            }
            totalFrameCount++;
        }
//
//        if (resp < 0) {
//            break;
//        }
    }
    av_write_trailer(ad->pOutFormatContext);

    end:
    fclose(ad->outFile);

    if (rs->srcData)
        av_freep(&rs->srcData[0]);
    av_freep(&rs->srcData);

    if (rs->srcData)
        av_freep(&rs->srcData[0]);
    av_freep(&rs->srcData);

    swr_free(&rs->resampleCtx);
    return resp < 0;
}
//
//int resampleAudio(bool showFrameData) {
//    int resp;
//
//    resp = avformat_write_header(ad->pOutFormatContext, nullptr);
//    if (resp < 0) {
//        cerr << "Error when writing header" << endl;
//        return -1;
//    }
//    while (1) {
//        const int outputFrameSize = ad->pOutCodecContext->frame_size;
//        int finished = 0;
//        while (av_audio_fifo_size(ad->avBuffer) < outputFrameSize) {
//            /* Decode one frame worth of audio samples, convert it to the
//             * output sample format and put it into the FIFO buffer.d
//             */
//            if (read_decode_convert_and_store(&finished)) {
//                goto end;
//            }
//            /**
//            * If we are at the end of the input file, we continue
//            * encoding the remaining audio samples to the output file.
//            */
//            if (finished)
//                break;
//        }
//        while (av_audio_fifo_size(ad->avBuffer) >= outputFrameSize ||
//               (finished && av_audio_fifo_size(ad->avBuffer) > 0)) {
//            /**
//              * Take one frame worth of audio samples from the FIFO buffer,
//              * encode it and write it to the output file.
//              */
//            if (load_encode_and_write() != 0)
//                goto end;
//        }
//
//        /**
//          * If we are at the end of the input file and have encoded
//          * all remaining samples, we can exit this loop and finish.
//          */
//        if (finished) {
//            int data_written;
//            /** Flush the encoder as it may have delayed frames. */
//            do {
//                if (encode_audio_frame(&data_written))
//                    goto end;
//            } while (data_written);
//            break;
//        }
//
//
//    }
//    /** Write the trailer of the output file container. */
//    resp = av_write_trailer(ad->pOutFormatContext);
//    if (resp < 0) {
//        cout << "error writing trailer" << endl;
//        goto end;
//    }
//    cout << "resampling succeded" << endl;
//    return 0;
//    end:
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
//int     read_decode_convert_and_store(int *finished) {
//
//    /** Temporary storage for the converted input samples. */
//    int **convertedInputSamples = nullptr;
//    int dataPresent;
//    int ret = AVERROR_EXIT;
//    /** Decode one frame worth of audio samples. */
//    if (decode_audio_frame(&dataPresent, finished)) {
//        goto end;
//    }
//    /**
//   * If we are at the end of the file and there are no more samples
//   * in the decoder which are delayed, we are actually finished.
//   * This must not be treated as an error.
//   */
//    if (*finished && !dataPresent) {
//        ret = 0;
//        goto end;
//    }
//    /** If there is decoded data, convert and store it */
//    if (dataPresent) {
//        /** Initialize the temporary storage for the converted input samples. */
//        if (init_converted_samples(&convertedInputSamples, ad->pOutCodecContext,
//                                   ad->pInFrame->nb_samples)) { goto end; }
//
//        /**
//   * Convert the input samples to the desired output sample format.
//   * This requires a temporary storage provided by converted_input_samples.
//   */
//        if (convert_samples((const uint8_t **) ad->pInFrame->extended_data, convertedInputSamples,
//                            ad->pInFrame->nb_samples,
//                            rs->resampleCtx)) { goto end; }
//
//
//
//        /** Add the converted input samples to the FIFO buffer for later processing. */
//        if (add_samples_to_fifo(ad->avBuffer, convertedInputSamples, ad->pInFrame->nb_samples)) { goto end; }
//
//        ret = 0;
//    }
//    ret = 0;
//
//    end:
//    if (convertedInputSamples) {
//        av_freep(&convertedInputSamples[0]);
//        free(convertedInputSamples);
//    }
//    av_frame_free(&ad->pInFrame);
//
//    return ret;
//
//}
//
//int decode_audio_frame(int *dataPresent, int *finished) {
//    int error;
//    /** Read one audio frame from the input file into a temporary packet. */
//    if ((error = av_read_frame(ad->pInFormatContext, ad->pPacket)) < 0) {
//        /** If we are the the end of the file, flush the decoder below. */
//        if (error == AVERROR_EOF)
//            *finished = 1;
//        else {
//
//            fprintf(stderr, "Could not read frame (error '%s')\n");
//
//            return error;
//
//        }
//    }
//
//    /**
//   * Decode the audio frame stored in the temporary packet.
//   * The input audio stream decoder is used to do this.
//   * If we are at the end of the file, pass an empty packet to the decoder
//   * to flush it.
//   */
//    if ((error = avcodec_decode_audio4(ad->pInCodecContext, ad->pInFrame, dataPresent, &ad->pPacket)) < 0) {
//        fprintf(stderr, "Could not decode frame (error '%s')\n");
//        // av_free_packet(ad->pPacket);
//        return error;
//    }
//
//    /**
//   * If the decoder has not been flushed completely, we are not finished,
//   * so that this function has to be called again.
//   */
//    if (*finished && *dataPresent)
//        *finished = 0;
//    av_free_packet(ad->pPacket);
//    return 0;
//}
//
//static int load_encode_and_write() {
//
//    /**
//     * Use the maximum number of possible samples per frame.
//     * If there is less than the maximum possible frame size in the FIFO
//     * buffer use this number. Otherwise, use the maximum possible frame size
//     */
//    const int frame_size = FFMIN(av_audio_fifo_size(ad->avBuffer),
//                                 ad->pOutCodecContext->frame_size);
//    int data_written;
//
//    /**
//       * Read as many samples from the FIFO buffer as required to fill the frame.
//       * The samples are stored in the frame temporarily.
//       */
//    if (av_audio_fifo_read(ad->avBuffer, (void **) ad->pInFrame->data, frame_size) < frame_size) {
//        fprintf(stderr, "Could not read data from FIFO\n");
//        av_frame_free(&ad->pInFrame);
//        return AVERROR_EXIT;
//    }
//
//    /** Encode one frame worth of audio samples. */
//    if (encode_audio_frame(ad->pInFrame, ad->pOutFormatContext,
//                           ad->pOutCodecContext, &data_written)) {
//        av_frame_free(&ad->pInFrame);
//        return AVERROR_EXIT;
//    }
//    av_frame_free(&ad->pInFrame);
//    return 0;
//}