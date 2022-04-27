#include <iostream>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern "C" {
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

void openFiles(const char *fpIn, const char *fpOut, FILE *fileIn, FILE *fileOut);

void initObjsForProcess(const AVCodec *pCodec, AVCodecParserContext *pParser, AVCodecContext *pCodecContext,
                        const enum AVCodecID *audio_id);

void showDataGetCodecId(AVFormatContext *pContext, bool printInfo, AVCodecID audioId, const char *inputFilePath);

using namespace std;

int main() {
    const char *inputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/inputRecording.wav";
    const char *outputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/outputRecording.wav";
    FILE *inFile = nullptr, *outFile = nullptr;

    AVFormatContext *pFormatContext = avformat_alloc_context();//alloc information for format of file

    avformat_open_input(&pFormatContext, inputFP, nullptr, nullptr);

    AVCodecID audioId;
    showDataGetCodecId(pFormatContext, true, audioId, inputFP);

    const AVCodec *pCodec = nullptr;
    AVCodecParserContext *pParser = nullptr;
    AVCodecContext *pCodecContext = nullptr;

    openFiles(inputFP, outputFP, inFile, outFile);
    initObjsForProcess(pCodec, pParser, pCodecContext, &audioId);

    //allocate memory for packet and frame readings
    AVPacket *pPacket = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();
    //TODO: all is allocated, now just need to fix while loop to loop over frames and perform edits on them
    /*
     * edits process:
     * 1: check if human voice detected -> if not, dont write the frame
     * need to think about removing frames from the start and end of the file - still need to think
     * 2: if not removing this current frame, need to clean up the audio in it
     */
    while (av_read_frame(pFormatContext, pPacket) >= 0) {
        //send raw data to codec through codec context
        // avcodec_send_packet(pCodecContext, pPacket);

        //recieve same raw data through codec context
        // avcodec_receive_frame(pCodecContext, pFrame);
    }
    end:
    fclose(inFile);
    fclose(outFile);

    avcodec_free_context(&pCodecContext);
    av_parser_close(pParser);
    av_frame_free(&pFrame);
    av_packet_free(&pPacket);
    cout << "succesfully exited program!" << endl;
    return 0;
}

/**
 * returns the correct av_codec_id while also printing data
 * @param pContext
 * @param printInfo boolean specifies whether you want to print: true = print
 * @param audioId the returning audio_codec_id
 * @param inputFilePath the input file path of the input file
 */
void showDataGetCodecId(AVFormatContext *pContext, bool printInfo, AVCodecID audioId, const char *inputFilePath) {
    const char *fileFormat = pContext->iformat->long_name;
    int64_t duration = pContext->duration;
    audioId = pContext->audio_codec_id;


    if (audioId == AV_CODEC_ID_NONE) {
        string path = (string) (inputFilePath);
        int loc = path.find('.');

        if (loc == -1) {
            cout << stderr << "ERROR: invalid file type" << endl;
            exit(1);
        }
        string ending = path.substr(loc);

        if (ending == ".wav") {
            audioId = AV_CODEC_ID_GSM_MS;
        } else if (ending == ".mp3") {
            audioId = AV_CODEC_ID_MP3;
        } else {
            cout << stderr << "ERROR: not found audio ending" << endl;
            exit(1);
        }
    }
    if (printInfo) {
        cout << "format: " << fileFormat << " duration: " << duration << endl;
        cout << "audio_codec_id: " << audioId << endl;
    }


}

/**
 * initializes all of the background objects for ffmpeg library
 * breaks if cannot initialize a certain object
 * @param pCodec
 * @param pParser
 * @param pCodecContext
 * @param audio_id
 */
void initObjsForProcess(const AVCodec *pCodec, AVCodecParserContext *pParser, AVCodecContext *pCodecContext,
                        const AVCodecID *audio_id) {
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

/**
 * creates file objects for the input and output files
 * @param fpIn input filepath
 * @param fpOut output filepath
 * @param fileIn input file object
 * @param fileOut output file object
 */
void openFiles(const char *fpIn, const char *fpOut, FILE *fileIn, FILE *fileOut) {
    fileIn = fopen(fpIn, "rb");
    fileOut = fopen(fpOut, "wb");

    if (fileIn == nullptr || fileOut == nullptr) {
        cout << stderr << "ERROR: could not open files" << endl;
        fclose(fileIn);
        fclose(fileOut);
        exit(1);
    }
}
