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
    //AVCodecParserContext *pParser = nullptr;
    AVCodecContext *pCodecContext = nullptr;
    AVCodecParameters *pCodecParam = nullptr;
    AVPacket *pPacket = nullptr;
    AVFrame *pFrame = nullptr;


    AudioDecoder(const char *inFilePath, const char *ptrOutFilePath);

/**
 * intializes:
 *  AVFormatContext *pFormatContext
    const AVCodec *pCodec
    AVCodecParserContext *pParser
    AVCodecContext *pCodecContext
    AVCodecParameters *pCodecParam
    AVPacket *pPacket
    AVFrame *pFrame
 */
    void initializeAllObjects();

/**
 * closes the input and output files
 * frees the codec, parser, frame, packet
 */
    void closeAllObjects();

/**
 * saves the audio using the first channel provided
 */
    void saveAudioFrame();

private:
/**
 * creates file objects for the input and output files
 * @param fpIn input filepath
 * @param fpOut output filepath
 * @param fileIn input file object
 * @param fileOut output file object
 */
    void openFiles();

    /**
     * inidializes the codec and its format and paramters
     * shows general info of the file
     * @param printInfo specify whether you want to show info of the file
     */
    void showDataGetCodec(bool printInfo);


};

#endif //FFMPEGTEST5_AUDIODECODER_H
