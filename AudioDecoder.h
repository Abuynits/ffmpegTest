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
    const char *inFilePath, *outFilePath;
    FILE *inFile = nullptr, *outFile = nullptr;
    AVFormatContext *pFormatContext = nullptr;
    const AVCodec *pCodec = nullptr;
    AVCodecParserContext *pParser = nullptr;
    AVCodecContext *pCodecContext = nullptr;
    AVCodecParameters *pCodecParam = nullptr;
    AVPacket *pPacket = nullptr;
    AVFrame *pFrame = nullptr;


    AudioDecoder(const char *inFilePath, const char *ptrOutFilePath);

    void initializeAllObjects();

    void closeAllObjects();

private:
    void openFiles(const char *fpIn, const char *fpOut, FILE *fileIn, FILE *fileOut);

    void showDataGetCodec(bool printInfo);


};

#endif //FFMPEGTEST5_AUDIODECODER_H
