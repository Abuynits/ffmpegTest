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

void showDataGetCodecId(AVFormatContext *pContext, bool printInfo, AVCodecID audioId, const char *inputFilePath);

int processAudioFrame(AVPacket *pPacket, AVCodecContext *pContext, AVFrame *pFrame, bool printFrameData, FILE *outfile);

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

    //codec = device able to decode or encode data
    pCodec = avcodec_find_decoder(audioId);
    //check if can open pCodec
    if (!pCodec) {
        cout << stderr << "ERROR: could not open pCodec" << endl;
        exit(1);
    }
    //try to open pParser -for parsing frames
    pParser = av_parser_init(pCodec->id);
    if (pParser == nullptr) {
        cout << stderr << "Parser not found" << endl;
        exit(1);
    }
    //get the context of the audio pCodec- hold info for encode/decode process
    pCodecContext = avcodec_alloc_context3(pCodec);
    if (pCodecContext == nullptr) {
        cout << stderr << "Could not allocate audio pCodec context" << endl;
        exit(1);
    }
    //open the actual pCodec:
    if (avcodec_open2(pCodecContext, pCodec, nullptr) < 0) {
        cout << stderr << "Could not open pCodec" << endl;
        exit(1);
    }
    //allocate memory for packet and frame readings
    AVPacket *pPacket = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();
    /*
     * edits process:
     * 1: check if human voice detected -> if not, dont write the frame
     * need to think about removing frames from the start and end of the file - still need to think
     * 2: if not removing this current frame, need to clean up the audio in it
     */

    while (av_read_frame(pFormatContext, pPacket) >= 0) {

        int response = processAudioFrame(pPacket, pCodecContext, pFrame, true, outFile);
        if (response < 0) {
            cout << stderr << "ERROR: broken processor, return value: " << response << endl;
            exit(1);
        }
        //clear the packet after each frame
        av_packet_unref(pPacket);
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

int
processAudioFrame(AVPacket *pPacket, AVCodecContext *pContext, AVFrame *pFrame, bool printFrameData, FILE *outfile) {
    //send raw data packed to decoder
    int resp = avcodec_send_packet(pContext, pPacket);
    if (resp < 0) {
        //check if error while sending packet to decoder
        return resp;
    }
    while (resp >= 0) {
        int counter = 0;
        getAFrame:
        resp = avcodec_receive_frame(pContext, pFrame);
        //if have an error code while getting a frame from a packet, break
        //TODO: getting AVERROR(EAGAIN)
        /*
         * FIX:
         * When you call avcodec_receive_frame and get EAGAIN error that means your decoder does not get enough data to decode
         * (e.x you send a B-frame in video).
         * So each time you get that error you should ignore it and go to next avcodec_send_packet
         * https://stackoverflow.com/questions/55354120/ffmpeg-avcodec-receive-frame-returns-averroreagain
         */
        if (resp == AVERROR(EAGAIN)) {
            counter++;
            cout << "Not enough data in frame, skipping to next packet, counter:" << counter << endl;
            //decoded not have enough data to process frame
            //not error unless reached end of the stream - pass more packets untill have enough to produce frame
            av_frame_unref(pFrame);
            av_freep(pFrame);
            //getting another packet
            return 0;
        } else if (resp < 0) {
            // Failed to get a frame from the decoder
            av_frame_unref(pFrame);
            av_freep(pFrame);
            cout << stderr << " Error while getting frame from codec: %s" << av_err2str(resp) << endl;
            return resp;
        }
        cout << "enough data to process!" << endl;
        if (printFrameData) {
            cout << "frame number: " << pContext->frame_number
                 << ", Pkt_Size: " << pFrame->pkt_size
                 << ", Pkt_pts: " << pFrame->pts
                 << ", Pkt_keyFrame: " << pFrame->key_frame << endl;

        }
        resp = av_get_bytes_per_sample(pContext->sample_fmt);

        if (resp < 0) {
            cout << stderr << " ERROR: cannot get data size" << endl;
        }
        //TODO: HAVE THE DECODED DATA, NOW NEED TO PROCESS THE WHOLE THING
        for (int i = 0; i < pFrame->nb_samples; i++)
//            for (int ch = 0; ch < pContext->ch_layout.nb_channels; ch++)//NOTE: original line
            for (int ch = 0; ch < pContext->channel_layout; ch++)
                fwrite(pFrame->data[ch] + resp * i, 1, reinterpret_cast<size_t>(pFrame), outfile);
    }


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
    //TODO: need to check wether the lenght of streams is >0- if it is, show error, but continue the run- want only 1 audio stream
    if( pContext->nb_streams>1){
        cout<<"CAUTION: detected more than 1 streams \t using streams[0]"<<endl;
    }
    AVCodecParameters *param = pContext->streams[0]->codecpar;

    if (param->codec_id == AV_CODEC_ID_NONE) {
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
