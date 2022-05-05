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

void showDataGetCodecId(AVFormatContext *pContext, bool printInfo, const char *inputFilePath, const AVCodec *pCodec,
                        AVCodecParameters *pCodecParameters);

int processAudioFrame(AVPacket *pPacket, AVCodecContext *pContext, AVFrame *pFrame, bool printFrameData, FILE *outfile);

void saveAudioFrame();

using namespace std;

int main() {
    //TODO: make sure that first can read and write files without any changes. Then go from there
    const char *inputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/inputRecording.wav";
    const char *outputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/outputRecording.wav";
    FILE *inFile = nullptr, *outFile = nullptr;
    //hold the header information from the format (file)
    // http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html
    AVFormatContext *pFormatContext = avformat_alloc_context();//alloc information for format of file
    if (!pFormatContext) {
        cout << stderr << "ERROR could not allocate memory for Format Context" << endl;
        return -1;
    }
    //open the file and read header - codecs not opened
    //AVFormatContext (what allocate memory for)
    //url to file
    //AVINputFormat -give Null and it will do auto detect
    // http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49
    if (avformat_open_input(&pFormatContext, inputFP, nullptr, nullptr) != 0) {
        cout << stderr << " ERROR: could not open file" << endl;
        exit(1);
    }
    // read Packets from the Format to get stream information
    // this function populates pFormatContext->streams
    // (of size equals to pFormatContext->nb_streams)
    // the arguments are:
    // the AVFormatContext
    // and options contains options for codec corresponding to i-th stream.
    // On return each dictionary will be filled with options that were not found.
    // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb
    if (avformat_find_stream_info(pFormatContext, nullptr) < 0) {
        cout << stderr << " ERROR could not get the stream info" << endl;
        exit(1);
    }
    const AVCodec *pCodec = nullptr;
    AVCodecParserContext *pParser = nullptr;
    AVCodecContext *pCodecContext = nullptr;
    AVCodecParameters *pCodecParam = nullptr;
//    showDataGetCodecId(pFormatContext, true, inputFP, pCodec, pCodecParam);
    //-----------------------------
    if (pFormatContext->nb_streams > 1) {
        cout << "CAUTION: detected more than 1 streams \t using streams[0]" << endl;
    }

    pCodecParam = pFormatContext->streams[0]->codecpar;
    // finds the registered decoder for a codec ID
    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
    pCodec = avcodec_find_decoder(pCodecParam->codec_id);

    if (pCodec == nullptr) {
        pCodecParam = nullptr;
        cout << stderr << " ERROR unsupported codec!" << endl;
        // In this example if the codec is not found we just skip it
    }
    bool printInfo = true;
    if (printInfo) {
        const char *fileFormat = pFormatContext->iformat->long_name;
        int64_t duration = pFormatContext->duration;

        cout << "format: " << fileFormat << " duration: " << duration << endl;
        cout << "audio_codec_id: " << pCodecParam->codec_id << endl;
    }

    //--------------------

    openFiles(inputFP, outputFP, inFile, outFile);
    //    AVCodecID temp = pCodecContext->codec_id;
    //codec = device able to decode or encode data
    // pCodec = avcodec_find_decoder(temp);
    //check if can open pCodec
    if (pCodec == nullptr) {
        cout << stderr << " ERROR: could not find pCodec ";
        exit(1);
    }

    //get the context of the audio pCodec- hold info for encode/decode process
    pCodecContext = avcodec_alloc_context3(pCodec);
    if (!pCodecContext) {
        cout << stderr << "Could not allocate audio pCodec context" << endl;
        exit(1);
    }

    // Fill the codec context based on the values from the supplied codec parameters
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
    if (avcodec_parameters_to_context(pCodecContext, pCodecParam) < 0) {
        cout << stderr << "failed to copy codec params to codec context";
        return -1;
    }
    //open the actual pCodec:
    if (avcodec_open2(pCodecContext, pCodec, nullptr) < 0) {
        cout << stderr << "Could not open pCodec" << endl;
        exit(1);
    }

    //allocate memory for packet and frame readings
    // https://ffmpeg.org/doxygen/trunk/structAVFrame.html
    AVPacket *pPacket = av_packet_alloc();
    if (!pPacket) {
        cout << stderr << "Could not open packet" << endl;
        exit(1);
    }
    //allocate memory for frame from readings
    // https://ffmpeg.org/doxygen/trunk/structAVPacket.html
    AVFrame *pFrame = av_frame_alloc();
    if (!pFrame) {
        cout << stderr << "Could not open frame" << endl;
        exit(1);
    }
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

int
processAudioFrame(AVPacket *pPacket, AVCodecContext *pContext, AVFrame *pFrame, bool printFrameData, FILE *outfile) {
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
        if (resp == AVERROR(EAGAIN) || resp == AVERROR_EOF) {
            cout << "Not enough data in frame, skipping to next packet" << endl;
            break;
        } else if (resp < 0) {
            cout << "Error while receiving a frame from the decoder: " << av_err2str(resp) << endl;
            return resp;
        }
        cout << "enough data to process!" << endl;
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

void saveAudioFrame(AVFrame *pFrame, FILE *outFile) {
    unsigned char *buf = pFrame->data[0];
    int wrap = pFrame->linesize[0];
    int xSize = pFrame->width;
    int ySize = pFrame->height;

    cout << "writing to file" << endl;
    for (int i = 0; i < ySize; i++) {
        fwrite(buf + i * wrap, 1, xSize, outFile);
    }
    fclose(outFile);
}

/**
 * returns the correct av_codec_id while also printing data
 * @param pContext
 * @param printInfo boolean specifies whether you want to print: true = print
 * @param audioId the returning audio_codec_id
 * @param inputFilePath the input file path of the input file
 */
void showDataGetCodecId(AVFormatContext *pContext, bool printInfo, const char *inputFilePath, const AVCodec *pCodec,
                        AVCodecParameters *pCodecParameters) {

    if (pContext->nb_streams > 1) {
        cout << "CAUTION: detected more than 1 streams \t using streams[0]" << endl;
    }

    *pCodecParameters = *pContext->streams[0]->codecpar;
    // finds the registered decoder for a codec ID
    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
    pCodec = avcodec_find_decoder(pCodecParameters->codec_id);

    if (pCodec == nullptr) {
        pCodecParameters = nullptr;
        cout << stderr << " ERROR unsupported codec!" << endl;
        // In this example if the codec is not found we just skip it
    }
    if (printInfo) {
        const char *fileFormat = pContext->iformat->long_name;
        int64_t duration = pContext->duration;

        cout << "format: " << fileFormat << " duration: " << duration << endl;
        cout << "audio_codec_id: " << pCodecParameters->codec_id << endl;
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
