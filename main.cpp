extern "C" {
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/timestamp.h>
#include <libavutil/samplefmt.h>
}

#include "AudioDecoder.h"
#include "AudioFilter.h"
#include "OutputAnalysis.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int loopOverPacketFrames(bool showFrameData);

int filterAudioFrame();

int transferStreamData(int *inputPts);

int applyFilters();

int applyMuxers();

void getAudioInfo();

using namespace std;

AudioDecoder *ad;
AudioFilter *av;
OutputAnalysis *audioInfo;

const char *inputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/inputRecording.wav";
const char *tempFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/outputRecording.wav";
const char *finalFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/finalOutput.wav";
const char *statOutFP = "/Users/abuynits/CLionProjects/ffmpegTest5/output.txt";

int totalFrameCount = 0;
const bool showData = false;

int main() {
    int resp;

    audioInfo = new OutputAnalysis(statOutFP);

    ad = new AudioDecoder(inputFP, tempFP, true, true);
    ad->openFiles();
    ad->initializeAllObjects();

    if (resp < 0) {
        cerr << "error: could not initialize decoder" << endl;
        return 1;
    }
    cerr << "initialized AudioDecoder" << endl;

    av = new AudioFilter(ad);
    resp = av->initializeAllObjets();
    if (resp < 0) {
        cerr << "error: could not initialize filters" << endl;
        return 1;
    }
    cerr << "initialized AudioFilter\n" << endl;

    resp = applyFilters();
    if (resp == 0) {
        cerr << "finished processing first loop" << endl;
    }

    ad->closeAllObjects();
    av->closeAllObjects();

    audioInfo->setFrameVals(ad->startFrame, ad->endFrame, totalFrameCount);

    //========================SECOND STAGE: make playable by wav output file==========================
    ad = new AudioDecoder(tempFP, finalFP, false, true);
    ad->openFiles();
    ad->initializeAllObjects();

    resp = applyMuxers();
    if (resp == 0) {
        cerr << "finished muxing files" << endl;
    }

    ad->closeAllObjects();
//    //==============RMS PROCESSING===================
    getAudioInfo();
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

int applyFilters() {
    int resp = 0;
    resp = avformat_write_header(ad->pOutFormatContext, nullptr);
    if (resp < 0) {
        cerr << "Error when writing header" << endl;
        return -1;
    }

    while (av_read_frame(ad->pInFormatContext, ad->pPacket) >= 0) {
        resp = loopOverPacketFrames(showData);
        if (resp < 0) {
            break;
        }
    }

    //flush the audio decoder
    ad->pPacket = nullptr;
    loopOverPacketFrames(showData);

    av_write_trailer(ad->pOutFormatContext);


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