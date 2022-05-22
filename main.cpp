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
    //loop over the frames in each packet and apply a chain of filters to each frame
    resp = applyFilters(writeFileHeader);
    if (resp == 0) {
        cerr << "finished processing first loop" << endl;
    }

    //close all of the objects
    ad->closeAllObjects();
    av->closeAllObjects();
    //save the stats from the filter stage into the info object
    audioInfo->setFrameVals(ad->startFrame, ad->endFrame, totalFrameCount);

    cout << "=====================DONE WITH SECOND LOOP=====================" << endl;
    //========================SECOND STAGE: make playable by wav output file==========================
    ad = new AudioDecoder(tempFP, finalFP, false, true);
    ad->openFiles();
    ad->initializeAllObjects();

    //loop over the file to get the correct header to be playable
    resp = applyMuxers();
    if (resp == 0) {
        cerr << "finished muxing files" << endl;
    }

    ad->closeAllObjects();
//    //==============RMS PROCESSING===================
//get the stats and show them
    getAudioInfo();

    cout << "=====================DONE WITH THIRD LOOP=====================" << endl;
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
    int resp = avcodec_send_packet(ad->pCodecContext, ad->pPacket);
    if (resp < 0) {
        cerr << "error submitting a packet for decoding: " << av_err2str(resp);
        return resp;
    }

    while (resp >= 0) {
        resp = avcodec_receive_frame(ad->pCodecContext, ad->pFrame);
        if (resp == AVERROR(EAGAIN)) {
            if (showFrameData) cerr << "Not enough data in frame, skipping to next packet" << endl;
            //decoded not have enough data to process frame
            //not error unless reached end of the stream - pass more packets untill have enough to produce frame
            clearFrames:
            av_frame_unref(ad->pFrame);
            av_freep(ad->pFrame);
            break;
        } else if (resp == AVERROR_EOF) {
            cerr << "Reached end of file" << endl;
            goto clearFrames;
        } else if (resp < 0) {
            cerr << "Error while receiving a frame from the decoder: " << av_err2str(resp) << endl;
            // Failed to get a frame from the decoder
            av_frame_unref(ad->pFrame);
            av_freep(ad->pFrame);
            return resp;
        }
        /*
         * TODO: need to find the noise level of audio file
         * try to look at astats filter, then at the portions where silence is detected idk
         * need to get the RMS factor: what kolya talk about
         */

        if (showFrameData)
            cerr << "frame number: " << ad->pCodecContext->frame_number
                 << ", Pkt_Size: " << ad->pFrame->pkt_size
                 << ", Pkt_pts: " << ad->pFrame->pts
                 << ", Pkt_keyFrame: " << ad->pFrame->key_frame << endl;

        if (filterAudioFrame() < 0) {
            cerr << "error in filtering" << endl;

        }

        av_frame_unref(ad->pFrame);
        av_freep(ad->pFrame);
        if (resp < 0) {
            return resp;
        }
        totalFrameCount++;
    }
    return 0;

}

int filterAudioFrame() {
    //add to source frame:
    int resp = av_buffersrc_add_frame(av->srcContext, ad->pFrame);
    //TODO:START
    if (resp < 0) {
        cerr << "Error: cannot send to graph: " << av_err2str(resp) << endl;
        breakFilter:
        av_frame_unref(ad->pFrame);
        return resp;
    }
    //get back the filtered data:
    while ((resp = av_buffersink_get_frame(av->sinkContext, ad->pFrame)) >= 0) {

        if (resp < 0) {
            cerr << "Error filtering data " << av_err2str(resp) << endl;
            goto breakFilter;
        }

        ad->saveAudioFrame(showData);

        if (resp < 0) {
            cerr << "Error muxing packet" << endl;
        }
        av_frame_unref(ad->pFrame);
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

int resampleAudio(bool showFrameData) {
    int resp;
    while (av_read_frame(ad->pInFormatContext, ad->pPacket) >= 0) {
        resp = avcodec_send_packet(ad->pCodecContext, ad->pPacket);
        if (resp < 0) {
            cerr << "error submitting a packet for decoding: " << av_err2str(resp);
            return resp;
        }
        cout<<"getting another packet"<<endl;

        while (resp >= 0) {
            resp = avcodec_receive_frame(ad->pCodecContext, ad->pFrame);
            if (resp == AVERROR(EAGAIN)) {
                if (showFrameData) cerr << "Not enough data in frame, skipping to next packet" << endl;
                //decoded not have enough data to process frame
                //not error unless reached end of the stream - pass more packets untill have enough to produce frame
                clearFrames:
                av_frame_unref(ad->pFrame);
                av_freep(ad->pFrame);
                break;
            } else if (resp == AVERROR_EOF) {
                cerr << "Reached end of file" << endl;
                goto clearFrames;
            } else if (resp < 0) {
                cerr << "Error while receiving a frame from the decoder: " << av_err2str(resp) << endl;
                // Failed to get a frame from the decoder
                av_frame_unref(ad->pFrame);
                av_freep(ad->pFrame);
                return resp;
            }
            /*
             * TODO: need to find the noise level of audio file
             * try to look at astats filter, then at the portions where silence is detected idk
             * need to get the RMS factor: what kolya talk about
             */

            //TODO: use this link for converting audio types: https://ffmpeg.org/doxygen/2.2/transcode_aac_8c-example.html


            if (showFrameData)
                cerr << "frame number: " << ad->pCodecContext->frame_number
                     << ", Pkt_Size: " << ad->pFrame->pkt_size
                     << ", Pkt_pts: " << ad->pFrame->pts
                     << ", Pkt_keyFrame: " << ad->pFrame->key_frame << endl;

            rs->numDstSamples = av_rescale_rnd(swr_get_delay(rs->resampleCtx, rs->numSrcSamples) + rs->numSrcSamples,
                                               ad->pCodecContext->sample_rate, ad->pCodecContext->sample_rate,
                                               AV_ROUND_UP);
            if (rs->numDstSamples > rs->maxDstNumSamples) {
                av_freep(&rs->dstData[0]);
                resp = av_samples_alloc(rs->dstData, &rs->dstLineSize, rs->numDstChannels,
                                        rs->numDstSamples, ad->pCodecContext->sample_fmt, 1);
                if (resp < 0)
                    cout << "cannot allocate samples" << endl;
                break;
                rs->maxDstNumSamples = rs->numDstSamples;

            }
            resp = swr_convert(rs->resampleCtx, rs->dstData, rs->numDstSamples, (const uint8_t **) rs->srcData,
                               rs->numSrcSamples);
            if (resp < 0) {
                cout << "ERRORO: converting samples" << endl;
                return -1;
            }
            rs->dstBufferSize = av_samples_get_buffer_size(&rs->dstLineSize, rs->numDstChannels, resp,
                                                           ad->pCodecContext->sample_fmt, 1);
            if (rs->dstBufferSize < 0) {
                cout << "ERROR: could not get sample buffer size" << endl;
                return -1;
            }
            fwrite(rs->dstData[0], 1, rs->dstBufferSize, ad->outFile);

        }

    }
//    const char *fmt;
//    if ((resp = ad->getSampleFmtFormat(&fmt, ad->pCodecContext->sample_fmt))) {
//        cout << "Error while getting sample format" << endl;
//        return -1;
//    }
    // av_channel_layout_describe(&rs->, buf, sizeof(buf));
//    fprintf(stderr, "Resampling succeeded. Play the output file with the command:\n"
//                    "ffplay -f %s -channel_layout %s -channels %d -ar %d %s\n");
    cout << "resampling succeded" << endl;
    return 0;
}