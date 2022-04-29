#include <iostream>


#include <stdio.h>
#include "AudioDecoder.h"


extern "C" {
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

void loopOverPackets(AVFormatContext *pFormatContext, AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame,
                     bool printFrameData, FILE *outFile);

int processAudioFrame(AVPacket *pPacket, AVCodecContext *pContext, AVFrame *pFrame, bool printFrameData, FILE *outfile);

using namespace std;

int main() {
    const char *inputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/inputRecording.wav";
    const char *outputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/outputRecording.wav";

    AudioDecoder decoder(inputFP, outputFP);

    decoder.initializeAllObjects();

    loopOverPackets(decoder.pFormatContext, decoder.pPacket, decoder.pCodecContext, decoder.pFrame, true,
                    decoder.outFile);

    decoder.closeAllObjects();
    cout << "succesfully exited program!" << endl;
    
    return 0;
}


void loopOverPackets(AVFormatContext *pFormatContext, AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame,
                     bool printFrameData, FILE *outFile) {

    int response = 0;
    int how_many_packets_to_process = 8;

    // fill the Packet with data from the Stream
    // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga4fdb3084415a82e3810de6ee60e46a61
    while (av_read_frame(pFormatContext, pPacket) >= 0) {
        response = processAudioFrame(pPacket, pCodecContext, pFrame, true, outFile);
        if (response < 0)
            break;
        // stop it, otherwise we'll be saving hundreds of frames
        if (--how_many_packets_to_process <= 0) break;

    }
    // https://ffmpeg.org/doxygen/trunk/group__lavc__packet.html#ga63d5a489b419bd5d45cfd09091cbcbc2
    av_packet_unref(pPacket);
}

int processAudioFrame(AVPacket *pPacket, AVCodecContext *pContext, AVFrame *pFrame, bool printFrameData,
                      FILE *outfile) {
    //send raw data packed to decoder
    int resp = avcodec_send_packet(pContext, pPacket);
    if (resp < 0) {
        //check if error while sending packet to decoder
        return resp;
    }
    while (resp >= 0) {
        // Return decoded output data (into a frame) from a decoder
        // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c
        resp = avcodec_receive_frame(pContext, pFrame);
        if (resp == AVERROR(EAGAIN)) {
            cout << "Not enough data in frame, skipping to next packet" << endl;
            //decoded not have enough data to process frame
            //not error unless reached end of the stream - pass more packets untill have enough to produce frame
            clearFrames:
            av_frame_unref(pFrame);
            av_freep(pFrame);
            break;
        } else if (resp == AVERROR_EOF) {
            cout << "Reached end of file" << endl;
            goto clearFrames;
        } else if (resp < 0) {
            cout << "Error while receiving a frame from the decoder: " << av_err2str(resp) << endl;
            // Failed to get a frame from the decoder
            av_frame_unref(pFrame);
            av_freep(pFrame);
            return resp;
        }
        if (printFrameData) {
            cout << "frame number: " << pContext->frame_number
                 << ", Pkt_Size: " << pFrame->pkt_size
                 << ", Pkt_pts: " << pFrame->pts
                 << ", Pkt_keyFrame: " << pFrame->key_frame << endl;

        }
        //TODO: only focussing on reading the files
    }

    return 0;
}