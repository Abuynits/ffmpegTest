#include <iostream>

using namespace std;

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

using namespace std;

const static char *inputFilePath = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/inputRecording.wav";
const static char *outputFilePath = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/outputRecording.wav";

int main() {
    //allocacte memory to avformat context that hold header information
    AVFormatContext *pFormatContext = avformat_alloc_context();
    //open file and read header and fill avformatcontext with min info about the file - codecs are not opened -
    avformat_open_input(&pFormatContext, inputFilePath, NULL, NULL);
    //can now print the format name and media duration:
    printf("Format %s, duration %lld us", pFormatContext->iformat->long_name, pFormatContext->duration);
    //now loop over all of the streams:
    for (int i = 0; i < pFormatContext->nb_streams; i++) {
        //for each stream, keep the parameters - describe properties of codec used
        AVCodecParameters *pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
        //can look up the proper codec to use with avcodec_find_decoder
        //find the registeed decoder for the cdedc id and return avCodec - componenet that knows how to encode/decode stream
        AVCodec *pLocalCodec = const_cast<AVCodec *>(avcodec_find_decoder(pLocalCodecParameters->codec_id));
        //here can split into video and audio: I will only focus on audio:
        if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            cout << "error: detected video" << endl;
            return 1;
        } else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            printf("Audio Codec: %d channels, sample rate %d", pLocalCodecParameters->channels,
                   pLocalCodecParameters->sample_rate);
        }
        //general info about the codec that is the same for audio and video (should only be about audio)
        printf("\tCodec %s ID %d bit_rate %lld", pLocalCodec->long_name, pLocalCodec->id,
               pLocalCodecParameters->bit_rate);

        //can allocate memory for AVCodecContext which hold context for decode/encode process - need to find it with CODEC params
        AVCodecContext *pCodecContext = avcodec_alloc_context3(pLocalCodec);
        //need to find a specific codec with our parameters
        avcodec_parameters_to_context(pCodecContext, pLocalCodecParameters);
        //open the codec
        avcodec_open2(pCodecContext, pLocalCodec, NULL);

        //allocate memory for AVPacket and AVFrame
        AVPacket *pPacket = av_packet_alloc();
        AVFrame *pFrame = av_frame_alloc();
        //feed packets while the stream has packets
        while (av_read_frame(pFormatContext, pPacket) >= 0) {
            //send raw data (compressed frame) to decoder
            avcodec_send_packet(pCodecContext, pPacket);
            //receive the raw data frame (uncompressed frame) from decoder
            avcodec_receive_frame(pCodecContext, pFrame);
            //print frame information:
            printf(
                    "Frame %c (%d) pts %d dts %d key_frame %d [coded_picture_number %d, display_picture_number %d]",
                    av_get_picture_type_char(pFrame->pict_type),
                    pCodecContext->frame_number,
                    pFrame->pts,
                    pFrame->pkt_dts,
                    pFrame->key_frame,
                    pFrame->coded_picture_number,
                    pFrame->display_picture_number
            );

        }
    }

    cout << "Hello, World!" << endl;
    return 0;
}
