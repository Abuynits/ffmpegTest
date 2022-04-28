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

AVCodecID showDataGetCodecId(AVFormatContext *pContext, bool printInfo, const char *inputFilePath);

int processAudioFrame(AVPacket *pPacket, AVCodecContext *pContext, AVFrame *pFrame, bool printFrameData, FILE *outfile);

using namespace std;

int main() {
    const char *inputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/inputRecording.wav";
    const char *outputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/outputRecording.wav";
    FILE *inFile = nullptr, *outFile = nullptr;

    AVFormatContext *pFormatContext = avformat_alloc_context();//alloc information for format of file

    avformat_open_input(&pFormatContext, inputFP, nullptr, nullptr);

    AVCodecID audioId;
    audioId = showDataGetCodecId(pFormatContext, true, inputFP);

    const AVCodec *pCodec = nullptr;
    AVCodecParserContext *pParser = nullptr;
    AVCodecContext *pCodecContext = nullptr;
    AVCodecParameters *pCodecParam = nullptr;

    openFiles(inputFP, outputFP, inFile, outFile);

    //codec = device able to decode or encode data
    pCodec = avcodec_find_decoder(audioId);
    //check if can open pCodec
    if (!pCodec) {
        cout << stderr << "ERROR: could not open pCodec" << endl;
        exit(1);
    }

    //get the context of the audio pCodec- hold info for encode/decode process
    pCodecContext = avcodec_alloc_context3(pCodec);
    if (pCodecContext == nullptr) {
        cout << stderr << "Could not allocate audio pCodec context" << endl;
        exit(1);
    }

    pCodecParam = pFormatContext->streams[0]->codecpar;
    // Fill the codec context based on the values from the supplied codec parameters
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
    if (avcodec_parameters_to_context(pCodecContext, pCodecParam) < 0) {
        cout << stderr << "failed to copy codec params to codec context";
        return -1;
    }
    //try to open pParser -for parsing frames
    pParser = av_parser_init(pCodec->id);
    if (pParser == nullptr) {
        cout << stderr << "Parser not found" << endl;
        exit(1);
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
//
//
//    /*
//     * edits process:
//     * 1: check if human voice detected -> if not, dont write the frame
//     * need to think about removing frames from the start and end of the file - still need to think
//     * 2: if not removing this current frame, need to clean up the audio in it
//     */
//
//    while (av_read_frame(pFormatContext, pPacket) >= 0) {
//
//        int response = processAudioFrame(pPacket, pCodecContext, pFrame, true, outFile);
//        if (response < 0) {
//            cout << stderr << "ERROR: broken processor, return value: " << response << endl;
//            exit(1);
//        }
//        //clear the packet after each frame
//        av_packet_unref(pPacket);
//    }

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


}

/**
 * returns the correct av_codec_id while also printing data
 * @param pContext
 * @param printInfo boolean specifies whether you want to print: true = print
 * @param audioId the returning audio_codec_id
 * @param inputFilePath the input file path of the input file
 */
AVCodecID showDataGetCodecId(AVFormatContext *pContext, bool printInfo, const char *inputFilePath) {
    const char *fileFormat = pContext->iformat->long_name;
    int64_t duration = pContext->duration;
    //TODO: need to check wether the lenght of streams is >0- if it is, show error, but continue the run- want only 1 audio stream
    if (pContext->nb_streams > 1) {
        cout << "CAUTION: detected more than 1 streams \t using streams[0]" << endl;
    }
    AVCodecParameters *param = pContext->streams[0]->codecpar;

    param->codec_id = AV_CODEC_ID_NONE;
    if (param->codec_id == AV_CODEC_ID_NONE) {
        string path = (string) (inputFilePath);
        int loc = path.find('.');

        if (loc == -1) {
            cout << stderr << "ERROR: invalid file type" << endl;
            exit(1);
        }
        string ending = path.substr(loc);

        if (ending == ".wav") {
            param->codec_id = AV_CODEC_ID_GSM_MS;
        } else if (ending == ".mp3") {
            param->codec_id = AV_CODEC_ID_MP3;
        } else {
            cout << stderr << "ERROR: not found audio ending" << endl;
            exit(1);
        }
    }
    if (printInfo) {
        cout << "format: " << fileFormat << " duration: " << duration << endl;
        cout << "audio_codec_id: " << param->codec_id << endl;
    }
    return param->codec_id;


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
