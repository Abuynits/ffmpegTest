#include <iostream>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavutil/frame.h>
#include <libavutil/mem.h>


void openFiles(const char *fpIn, const char *fpOut, FILE *fileIn, FILE *fileOut);

void initObjsForProcess(const AVCodec *pCodec, AVCodecParserContext *pParser, AVCodecContext *pCodecContext,
                        enum AVCodecID *audio_id);

using namespace std;

int main() {
    const char *inputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/inputRecording.wav";
    const char *outputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/outputRecording.wav";
    const char *desiredFileType = "wav";//CHANGE THIS DEPENDING ON WHAT READ
    FILE *inFile = nullptr, *outFile = nullptr;

    AVFormatContext *pFormatContext = avformat_alloc_context();//alloc information for format of file

    avformat_open_input(&pFormatContext, inputFP, nullptr, nullptr);
    const char *fileFormat = pFormatContext->iformat->long_name;
    int64_t duration = pFormatContext->duration;
    cout << "format: " << fileFormat << " duration: " << duration << endl;

    enum AVCodecID audio_id = pFormatContext->audio_codec_id;

    cout << "audio_codec_id: " << audio_id << endl;

    const AVCodec *pCodec;
    AVCodecParserContext *pParser;
    AVCodecContext *pCodecContext;

    initObjsForProcess(pCodec, pParser, pCodecContext, &audio_id);

    openFiles(inputFP, outputFP, inFile, outFile);

    //allocate memory for packet and frame readings
    AVPacket *pPacket = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();

    cout << "Hello, World!" << endl;
    return 0;
}

void initObjsForProcess(const AVCodec *pCodec, AVCodecParserContext *pParser, AVCodecContext *pCodecContext,
                        enum AVCodecID *audio_id) {
//codec = device able to decode or encode data
    pCodec = avcodec_find_decoder(*audio_id);
    //check if can open pCodec
    if (!pCodec) {
        cout << stderr << "ERROR: could not open pCodec" << endl;
        exit(1);
    }
    //try to open pParser -for parsing frames
    pParser = av_parser_init(pCodec->id);
    if (!pParser) {
        cout << stderr << "Parser not found" << endl;
        exit(1);
    }
    //get the context of the audio pCodec- hold info for encode/decode process
    pCodecContext = avcodec_alloc_context3(pCodec);
    if (!pCodecContext) {
        cout << stderr << "Could not allocate audio pCodec context" << endl;
        exit(1);
    }
    //open the actual pCodec:
    if (avcodec_open2(pCodecContext, pCodec, nullptr) < 0) {
        cout << stderr << "Could not open pCodec" << endl;
        exit(1);
    }
}

void openFiles(const char *fpIn, const char *fpOut, FILE *fileIn, FILE *fileOut) {
    fileIn = fopen(fpIn, "rb");
    fileOut = fopen(fpOut, "wb");

    if (fileIn == nullptr || fileOut == nullptr) {
        cout << "ERROR: could not open files" << endl;
        fclose(fileIn);
        fclose(fileOut);
        exit(1);
    }
}
