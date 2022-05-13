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
#include <libavutil/timestamp.h>
#include <libavutil/samplefmt.h>
}

class AudioDecoder {
public:
    const char *inputFP = nullptr;
    const char *outputFP = nullptr;
    FILE *inFile= nullptr;
    FILE *outFile= nullptr;

    AVFormatContext *pInFormatContext= nullptr;
    AVFormatContext *pOutFormatContext= nullptr;
    const AVOutputFormat *outputFormat = nullptr;
    const AVCodec *pCodec = nullptr;
    AVCodecContext *pCodecContext = nullptr;
    AVPacket *pPacket = nullptr;
    AVFrame *pFrame = nullptr;
    AVStream *audioStream = nullptr;
    int avStreamIndex = -1;
    int audioFrameCount = 0;
    int *streamMapping= nullptr;
    int demuxerStreamIndex =0;
    int streamMappingSize =0;
    AVStream *outStream=nullptr;
    AVStream  *inStream = nullptr;
    bool iCodec=false;
    bool iDemuxer = false;

    AudioDecoder(const char *inFilePath, const char *ptrOutFilePath,bool initCodecs,bool initDemuxer);

/**
 * intializes:
 *  AVFormatContext *pInFormatContext
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
    int saveAudioFrame();

/**
 * creates file objects for the input and output files
 */
    void openFiles();

    static int get_format_from_sample_fmt(const char **fmt, enum AVSampleFormat audioFormat);

private:


    /**
     * init the codec and tis parameters
     * @param mediaType default = audiotype
     * @return
     */
    int initCodec(enum AVMediaType mediaType = AVMEDIA_TYPE_AUDIO);

    int initDemuxer();
};

#endif //FFMPEGTEST5_AUDIODECODER_H