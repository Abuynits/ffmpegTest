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

/**
 * constructor adds file paths to the object
 * @param inFilePath input file path
 * @param ptrOutFilePath output file path
 */
    AudioDecoder(const char *inFilePath, const char *ptrOutFilePath);

/**
 *  initializes the following objects:
 * out and in files
 * AVFormatContext
 * AVCodec
 * AVCodecParserContext
 * AVCodecContext
 * AVCondecPArameters
 * AVPacket
 * AVFrame
 */
    void initializeAllObjects();

/**
 * closes all of the objects which include:
 * both files
 * av parser
 * av frame
 * av packet
 * av codec
 */
    void closeAllObjects();

private:
/**
 * creates file objects for the input and output files
 * @param fpIn input filepath
 * @param fpOut output filepath
 * @param fileIn input file object
 * @param fileOut output file object
 */
    void openFiles(const char *fpIn, const char *fpOut, FILE *fileIn, FILE *fileOut);

    /*
     * instantiates the avcodec, its parameters, and context
     * helper function
     */
    void showDataGetCodec(bool printInfo);

};

#endif //FFMPEGTEST5_AUDIODECODER_H
