#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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


int loopOverPacketFrames();


int getAudioRunCommand();

int filterAudioFrame();


using namespace std;

AudioDecoder *ad;
AudioFilter *av;

int main() {
    const char *inputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/inputRecording.wav";
    const char *outputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/outputRecording.wav";

    ad = new AudioDecoder(inputFP, outputFP);


//TODO: try to write without having a filter - just run through the buffers and see what you get.
    ad->openFiles();
    cout << "opened files" << endl;

    ad->initializeAllObjects();
    cout << "initialized all objects" << endl;
    av = new AudioFilter(ad);
    int resp = av->initializeAllObjets();
    if (resp < 0) {
        cout << "error: could not initialize filters" << endl;
        return 1;
    }
    cout << "initialized all filters\n" << endl;

    while (av_read_frame(ad->pFormatContext, ad->pPacket) >= 0) {
        resp = loopOverPacketFrames();
        if (resp < 0) {
            break;
        }

    }

    //flush the audio decoder
    ad->pPacket = nullptr;
    loopOverPacketFrames();


    resp = getAudioRunCommand();
    if (resp < 0) {
        cout << "ERROR getting ffplay command" << endl;
        goto end;
    }

    end:
    ad->closeAllObjects();
    av->closeAllObjects();

    return 0;
}

int getAudioRunCommand() {
    enum AVSampleFormat sampleFormat = ad->pCodecContext->sample_fmt;
    int channelNum = ad->pCodecContext->channels;
    const char *sFormat;

    if (av_sample_fmt_is_planar(sampleFormat)) {
        const char *packed = av_get_sample_fmt_name(sampleFormat);
        printf("Warning: the sample format the decoder produced is planar "
               "(%s). This example will output the first channel only.\n",
               packed ? packed : "?");
        sampleFormat = av_get_packed_sample_fmt(sampleFormat);
        channelNum = 1;
    }

    if (AudioDecoder::get_format_from_sample_fmt(&sFormat, sampleFormat) < 0) {
        return -1;
    }
    printf("Play the output audio file with the command:\n"
           "ffplay -f %s -ac %d -ar %d %s\n",
           sFormat, channelNum, ad->pCodecContext->sample_rate,
           ad->outputFP);
    return 0;
}


int loopOverPacketFrames() {
    int resp = avcodec_send_packet(ad->pCodecContext, ad->pPacket);
    if (resp < 0) {
        cout << "error submitting a packet for decoding: " << av_err2str(resp);
        return resp;
    }
    while (resp >= 0) {
        resp = avcodec_receive_frame(ad->pCodecContext, ad->pFrame);
        if (resp == AVERROR(EAGAIN)) {
            cout << "Not enough data in frame, skipping to next packet" << endl;
            //decoded not have enough data to process frame
            //not error unless reached end of the stream - pass more packets untill have enough to produce frame
            clearFrames:
            av_frame_unref(ad->pFrame);
            av_freep(ad->pFrame);
            break;
        } else if (resp == AVERROR_EOF) {
            cout << "Reached end of file" << endl;
            goto clearFrames;
        } else if (resp < 0) {
            cout << "Error while receiving a frame from the decoder: " << av_err2str(resp) << endl;
            // Failed to get a frame from the decoder
            av_frame_unref(ad->pFrame);
            av_freep(ad->pFrame);
            return resp;
        }

        cout << "frame number: " << ad->pCodecContext->frame_number
             << ", Pkt_Size: " << ad->pFrame->pkt_size
             << ", Pkt_pts: " << ad->pFrame->pts
             << ", Pkt_keyFrame: " << ad->pFrame->key_frame << endl;

        if (filterAudioFrame() < 0) {
            cout << "error in filtering" << endl;

        }
//        av_frame_unref(ad->pFrame);
//


       // resp = ad->saveAudioFrame();

        av_frame_unref(ad->pFrame);
        av_freep(ad->pFrame);
        if (resp < 0) {
            return resp;
        }
    }
    return 0;

}


int filterAudioFrame() {
    //add to source frame:
    int resp = av_buffersrc_add_frame(av->srcFilterContext, ad->pFrame);
    //TODO:START
    if (resp < 0) {
        cout << "Error: cannot send to graph: " << av_err2str(resp) << endl;
        breakFilter:
        av_frame_unref(ad->pFrame);
        return resp;
    }
    //get back the filtered data:
    while ((resp = av_buffersink_get_frame(av->sinkFilterContext, ad->pFrame)) >= 0) {

        if (resp < 0) {
            cout << "Error filtering data " << av_err2str(resp) << endl;
            goto breakFilter;
        }
        //TODO: unreference all audio information - lose object and write NOTHING - bug here!!
        ad->saveAudioFrame();
        av_frame_unref(ad->pFrame);
    }
    return 0;
}