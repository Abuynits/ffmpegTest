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
    //the input and output filepaths /file objects
    const char *inputFP = nullptr;
    const char *outputFP = nullptr;
    FILE *inFile = nullptr;
    FILE *outFile = nullptr;
    //input format context: used for input file path
    AVFormatContext *pInFormatContext = nullptr;
    //output format context: used for output file path
    AVFormatContext *pOutFormatContext = nullptr;
    //used to set the specific output format for the input format context when do 1st loop
    const AVOutputFormat *outputFormat = nullptr;
    //the codec used for decoding the input file
    const AVCodec *pCodec = nullptr;
    //the context of the codec: set from the format context and parameters
    AVCodecContext *pInCodecContext = nullptr;
    //the packet object used for looping over a audio file
    AVPacket *pPacket = nullptr;
    //the frames of the audio packet taht will be loped over
    AVFrame *pFrame = nullptr;
    //trasnfer the specific input stream between the in and out format context
    AVStream *audioStream = nullptr;
    //the specific index of the audio stream within a packet
    int avStreamIndex = -1;
    //the count of all audio frames that loop over
    int audioFrameCount = 0;
    //the array of demuxing information that is accesed with the stream index
    int *streamMapping = nullptr;
    //the specific audio stream index using in demuxing
    int demuxerStreamIndex = 0;
    //the size of the stream map
    int streamMappingSize = 0;
    //in and out streams used to transfer information between the in and out codecs
    AVStream *outStream = nullptr;
    AVStream *inStream = nullptr;
    //set the booleans depending on the function that you want to perform
    //first loop = both true, second loop = only demuxer is true
    bool iCodec = false;
    bool iDemuxer = false;
    //track the start and end frames: when start and end writing to the output file
    int startFrame, endFrame;
    //start writing when =0: skip the first frame to prevent writing the bad header in the first loop.
    int startWriting = -1;

    struct waveHeader {
        /* RIFF Header: "RIFF" */
        char riff_header[4];
        /* size of audio data + sizeof(struct wave_hdr) - 8 */
        int wav_size;
        /* "WAVE" */
        char wav_header[4];

        /* Format Header */
        /* "fmt " (includes trailing space) */
        char fmt_header[4];
        /* Should be 16 for PCM */
        int fmt_chunk_size;
        /* Should be 1 for PCM. 3 for IEEE Float */
        short audio_format;
        short num_channels;
        int sample_rate;
        /*
         * Number of bytes per second
         * sample_rate * num_channels * bit_depth/8
         */
        int byte_rate;
        /* num_channels * bytes per sample */
        short sample_alignment;
        /* bits per sample */
        short bit_depth;

        /* Data Header */
        /* "data" */
        char data_header[4];
        /*
         * size of audio
         * number of samples * num_channels * bit_depth/8
         */
        int data_bytes;
    } __attribute__((__packed__));

    /**
     * init audiodecoder
     * @param inFilePath input file path
     * @param ptrOutFilePath output file path
     * @param initCodecs boolean specify whether init codec
     * @param initDemuxer  boolean specify whether init demuxers
     */
    AudioDecoder(const char *inFilePath, const char *ptrOutFilePath, bool initCodecs, bool initDemuxer);
     void write_wave_hdr(int fd, size_t size)
    {
        struct waveHeader wh;

        memcpy(&wh.riff_header, "RIFF", 4);
        wh.wav_size = size + sizeof(struct waveHeader) - 8;
        memcpy(&wh.wav_header, "WAVE", 4);
        memcpy(&wh.fmt_header, "fmt ", 4);
        wh.fmt_chunk_size = 16;
        wh.audio_format = 1;
        wh.num_channels = 1;
        wh.sample_rate = pInCodecContext->sample_rate;
        wh.sample_alignment = 2;
        wh.bit_depth = 16;
        wh.byte_rate = wh.sample_rate * wh.sample_alignment;
        memcpy(&wh.data_header, "data", 4);
        wh.data_bytes = size;

        //write(fd, &wh, sizeof(struct waveHeader));
    }
    /**
     * init the format contexts
     * get the input format for the input context
     * set the output format for the input context
     * set the input context's stream info
     * set the output formats stream info
     * openInputFile and/or openOutputFile is called
     * allocate memory to packet and frame objects
     */
    void initializeAllObjects();

    /**
     * closes the input and output files
     * frees the codec, parser, frame, packet
     */
    void closeAllObjects();

    /**
     * saves the audio using the first channel provided
     * tracks the start and end frames of the audio stream
     * write RAW data: used only in first loop when processing frames
     * @param showFrameData boolean specifies whether to show frame data while saving
     * @return whether an error happened while saving
     */
    int saveAudioFrame(bool showFrameData);

    /**
     * creates file objects for the input and output files
     */
    void openFiles();

    /**
     * get the desired sample fmt from an audio context format
     * @param fmt the fmt where store the fmt that corresponds to audio context
     * @param audioFormat the format of the specific context
     * @return show whether found a format or not
     */
    static int getSampleFmtFormat(const char **fmt, enum AVSampleFormat audioFormat);

    /**
     * get the raw command to run the audio via ffmpeg command line
     * @return int say whether run the audio or not
     */
    int getAudioRunCommand();

private:

    /**
     * finds the best stream for the codec
     * finds the decoder that corresponds to this stream
     * makes context for the codec
     * sets the context's parameters from the format context
     * opens the codec
     * @param mediaType default = audiotype- the media for which get codec
     * @return whether error happened
     */
    int openInputFile(enum AVMediaType mediaType = AVMEDIA_TYPE_AUDIO);

    /**
     * sets a different input parameters to the input format context
     * sets the output format context as an output
     * loops over all of the streams in the input format context
     * formats the corresponding output streams in the output format context
     * opens the streams for writing during looping
     * @return whether error happened
     */
    int openOutputFile();
};

#endif //FFMPEGTEST5_AUDIODECODER_H