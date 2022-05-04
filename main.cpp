#include <iostream>


#include <stdio.h>
#include "AudioDecoder.h"
#include "AudioFilter.h"

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

/**
 * loops over the packets of a audio stream
 * @param ad the audiodecoder which holds all of the data
 * @param showData specifies whether you want to show data (true = yes)
 */
void loopOverPackets(AudioDecoder *ad, AudioFilter *av, bool showData);

/**
 * loops over the frames present in an individual audio packet
 * @param ad audiodecoder which contains all of the important objects
 * @param showData specifies whether you want to show data (true = yes)
 * @return value of whether you have a successful transfer, 0= success, <0= pain....
 */
int processAudioPacket(AudioDecoder *ad, AudioFilter *av, bool showData);

int filterAudioFrame(AVFrame *pFrame, AudioFilter *av, AudioDecoder *ad);

using namespace std;

int main() {
    const char *inputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/inputRecording.wav";
    const char *outputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/outputRecording.mp4";

    AudioDecoder decoder(inputFP, outputFP);

    decoder.initializeAllObjects();

    AudioFilter av(&decoder);

    int resp = av.initializeAllObjets();
    if (resp < 0) {
        cout << "error: could not initialize filters" << endl;
        return 1;
    }

    loopOverPackets(&decoder, &av, true);

    decoder.closeAllObjects();
    av.closeAllObjects();


    cout << "successfully converted file!" << endl;

    return 0;
}

void loopOverPackets(AudioDecoder *ad, AudioFilter *av, bool showData) {
    int response;
    int how_many_packets_to_process = 8;

    // fill the Packet with data from the Stream
    // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga4fdb3084415a82e3810de6ee60e46a61
    while (av_read_frame(ad->pFormatContext, ad->pPacket) >= 0) {
        response = processAudioPacket(ad, av, showData);
        if (response < 0)
            break;
        // stop it, otherwise we'll be saving hundreds of frames
        if (--how_many_packets_to_process <= 0) {
            cout << "reach end of file" << endl;
            break;

        }
        // https://ffmpeg.org/doxygen/trunk/group__lavc__packet.html#ga63d5a489b419bd5d45cfd09091cbcbc2
        av_packet_unref(ad->pPacket);
    }

}

int processAudioPacket(AudioDecoder *ad, AudioFilter *av, bool showData) {
    //send raw data packed to decoder
    int resp = avcodec_send_packet(ad->pCodecContext, ad->pPacket);
    if (resp < 0) {
        cout << "Error while receiving a frame from the decoder: " << av_err2str(resp) << endl;
        return resp;
    }
    while (resp >= 0) {
        // Return decoded output data (into a frame) from a decoder
        // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c
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
        if (showData) {
            cout << "frame number: " << ad->pCodecContext->frame_number
                 << ", Pkt_Size: " << ad->pFrame->pkt_size
                 << ", Pkt_pts: " << ad->pFrame->pts
                 << ", Pkt_keyFrame: " << ad->pFrame->key_frame << endl;

        }

        //TODO: process files here through filters!
        //TODO: problem with saving the audio.... not uploading for some reason
        ad->saveAudioFrame();
//        if (filterAudioFrame(ad->pFrame, av, ad) < 0) {
//            cout << "error in filtering" << endl;
//
//        }


    }

    return 0;
}

int filterAudioFrame(AVFrame *pFrame, AudioFilter *av, AudioDecoder *ad) {
    //add to source frame:
    int resp = av_buffersrc_write_frame(av->srcFilterContext, pFrame);
    //TODO:START
    //look at to fix/ adjust bug: https://stackoverflow.com/questions/61871719/ffmpeg-c-volume-filter
    //potentially do avfilter_graph_create_filter vs allocate context
    //maybe try to change how give input to volume filter:
    /*
     *
    snprintf(args, sizeof(args),"%f",2.0f);
     is all I need??
     */
    //TODO:END
    if (resp < 0) {
        cout << "Error: cannot send to graph: " << av_err2str(resp) << endl;
        breakFilter:
        av_frame_unref(pFrame);
        return resp;
    }
    //get back the filtered data:
    while ((resp = av_buffersink_get_frame(av->sinkFilterContext, pFrame)) >= 0) {

        if (resp < 0) {
            av_frame_unref(pFrame);
            cout << "Error filtering data " << av_err2str(resp) << endl;
            goto breakFilter;
        }
        //TODO: unreference all audio information - lose object and write NOTHING - bug here!!
        ad->saveAudioFrame();
        av_frame_unref(pFrame);
    }
    return 0;

}
