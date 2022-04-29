//
// Created by Alexiy Buynitsky on 4/29/22.
//

#ifndef FFMPEGTEST5_AUDIODECODER_H
#define FFMPEGTEST5_AUDIODECODER_H


#include <cstdio>
#include <iostream>

using namespace std;
extern "C" {
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class AudioDecoder {
public:
    char *inFilePath, *outFilePath;
    FILE *inFile = nullptr, *outFile = nullptr;
    AVFormatContext *pFormatContext;
    const AVCodec *pCodec;
    AVCodecParserContext *pParser;
    AVCodecContext *pCodecContext;
    AVCodecParameters *pCodecParam;
    AVPacket *pPacket;
    AVFrame *pFrame;

    AudioDecoder(char *inFilePath, char *ptrOutFilePath);

    void initializeAllObjects();

    void processData();

private:
    void openFiles(const char *fpIn, const char *fpOut, FILE *fileIn, FILE *fileOut);

    void loopOverPackets();

    int
    processAudioFrame(AVPacket *pPacket, AVCodecContext *pContext, AVFrame *pFrame, bool printFrameData,
                      FILE *outfile);

    void showDataGetCodec(bool printInfo);

};

#endif //FFMPEGTEST5_AUDIODECODER_H
