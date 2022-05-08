#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

void openFiles();

void initializeAllObjects();

int initCodec(enum AVMediaType mediaType);

void showDataGetCodecId(AVFormatContext *pContext, bool printInfo, const char *inputFilePath, const AVCodec *pCodec,
                        AVCodecParameters *pCodecParameters);

int processAudioFrame(AVPacket *pPacket, AVCodecContext *pContext, AVFrame *pFrame, bool printFrameData, FILE *outfile);

void saveAudioFrame(AVFrame *pFrame, FILE *outFile);

using namespace std;
const char *inputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/inputRecording.wav";
const char *outputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/outputRecording.mp4";
FILE *inFile;
FILE *outFile;
AVFormatContext *pFormatContext;
const AVCodec *pCodec = nullptr;
AVCodecContext *pCodecContext = nullptr;
AVCodecParameters *pCodecParam = nullptr;
AVPacket *pPacket = nullptr;
AVFrame *pFrame = nullptr;
AVStream *audioStream = nullptr;
int avStreamIndex = -1;

int main() {

    //++++++++++++++++++++++
    //PLAN: use https://fossies.org/linux/ffmpeg/doc/examples/demuxing_decoding.c
    //for setting up video processing. Once I can loop, merge with master to add audio filtering capabilities

    openFiles();
    cout << "opened files" << endl;
    initializeAllObjects();
    cout << "initialized all objects" << endl;


    int resp = initCodec(AVMEDIA_TYPE_AUDIO);
    if (resp == 0) {
        cout << "created codec" << endl;
    }

    audioStream = pFormatContext->streams[avStreamIndex];

    //dump input information to stderr
    av_dump_format(pFormatContext, 0, inputFP, 0);

    //allocate memory for frame from readings
    // https://ffmpeg.org/doxygen/trunk/structAVPacket.html
    pFrame = av_frame_alloc();
    if (!pFrame) {
        cout << stderr << "Could not open frame" << endl;
        exit(1);
    }
    //allocate memory for packet and frame readings
    // https://ffmpeg.org/doxygen/trunk/structAVFrame.html
    pPacket = av_packet_alloc();
    if (!pPacket) {
        cout << stderr << "Could not open packet" << endl;
        exit(1);
    }

    cout<<"reading to read files!"<<endl;

    return 0;

    // openFiles(inputFP, outputFP, &inFile, &outFile);
    //hold the header information from the format (file)
    // http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html

    pFormatContext = avformat_alloc_context();//alloc information for format of file
    if (pFormatContext == nullptr) {
        cout << stderr << "ERROR could not allocate memory for Format Context" << endl;
        exit(1);
    }


    //open the file and read header - codecs not opened
    //AVFormatContext (what allocate memory for)
    //url to file
    //AVINputFormat -give Null and it will do auto detect
    //try to get some information of the file vis
    // http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49
    //TODO: check if this has to be !=0, or <=0
    resp = avformat_open_input(&pFormatContext, inputFP, nullptr, nullptr);
    if (resp != 0) {
        cout << stderr << " ERROR: could not open file: " << av_err2str(resp) << endl;
        exit(1);
    }
    // read Packets from the Format to get stream information
    //if the fine does not have a ehader ,read some frames to figure out the information and storage type of the file
    // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb
    if (avformat_find_stream_info(pFormatContext, nullptr) < 0) {
        cout << stderr << " ERROR could not get the stream info" << endl;
        exit(1);
    }
    //    showDataGetCodecId(pFormatContext, true, inputFP, pCodec, pCodecParam);
    //-----------------------------
    if (pFormatContext->nb_streams > 1) {
        cout << "CAUTION: detected more than 1 streams \t using streams[0]" << endl;
        exit(1);
    }

    pCodecParam = pFormatContext->streams[0]->codecpar;
    // finds the registered decoder for a codec ID
    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
    pCodec = avcodec_find_decoder(pCodecParam->codec_id);
    if (pCodec == nullptr) {
        cout << stderr << " ERROR unsupported codec!" << endl;
        exit(1);
        // In this example if the codec is not found we just skip it
    }
//    bool printInfo = true;
//    if (printInfo) {
    const char *fileFormat = pFormatContext->iformat->long_name;
    int64_t duration = pFormatContext->duration;

    cout << "format: " << fileFormat << " duration: " << duration << endl;
    cout << "audio_codec_id: " << pCodecParam->codec_id << endl;


    //HERE
    // https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
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
        exit(1);
    }

    //open the actual pCodec:
    if (avcodec_open2(pCodecContext, pCodec, nullptr) < 0) {
        cout << stderr << "Could not open pCodec" << endl;
        exit(1);
    }
    //allocate memory for frame from readings
    // https://ffmpeg.org/doxygen/trunk/structAVPacket.html
    pFrame = av_frame_alloc();
    if (!pFrame) {
        cout << stderr << "Could not open frame" << endl;
        exit(1);
    }
    //allocate memory for packet and frame readings
    // https://ffmpeg.org/doxygen/trunk/structAVFrame.html
    pPacket = av_packet_alloc();
    if (!pPacket) {
        cout << stderr << "Could not open packet" << endl;
        exit(1);
    }



    // int packetsToProcess = 8;
    // fill the Packet with data from the Stream
    // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga4fdb3084415a82e3810de6ee60e46a61
    while (av_read_frame(pFormatContext, pPacket) >= 0) {
        resp = processAudioFrame(pPacket, pCodecContext, pFrame, true, outFile);
        if (resp < 0)
            break;
        // stop it, otherwise we'll be saving hundreds of frames
        //  packetsToProcess--;
        // if (packetsToProcess <= 0) { break; }
        av_packet_unref(pPacket);
        //TODO: use --------------------------------
        //https://ffmpeg.org/doxygen/3.3/decode__audio_8c_source.html
        //https://steemit.com/programming/@targodan/decoding-audio-files-with-ffmpeg
        //TODO: -----------------------------------

    }
    // https://ffmpeg.org/doxygen/trunk/group__lavc__packet.html#ga63d5a489b419bd5d45cfd09091cbcbc2

    end:
    fclose(inFile);
    fclose(outFile);
    avcodec_free_context(&pCodecContext);
    av_frame_free(&pFrame);
    av_packet_free(&pPacket);
    cout << "successfully exited program!" << endl;
    return 0;
}

int
processAudioFrame(AVPacket *pPacket, AVCodecContext *pContext, AVFrame *pFrame, bool printFrameData, FILE *outfile) {
    //send raw data packed to decoder
    int resp = avcodec_send_packet(pContext, pPacket);
    if (resp < 0) {
        //check if error while sending packet to decoder
        cout << "Error while receiving a frame from the decoder: " << av_err2str(resp) << endl;
        return resp;
    }
    while (resp >= 0) {
        // Return decoded output data (into a frame) from a decoder
        // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c
        resp = avcodec_receive_frame(pContext, pFrame);
        if (resp == AVERROR(EAGAIN)) {
            cout << "Not enough data in frame, skipping to next packet" << endl;
            //decoded not have enough data to process frame
            //not error unless reached end of the stream - pass more packets untill have enough to produce frame
            clearFrames:
            av_frame_unref(pFrame);
            av_freep(pFrame);
            break;
        } else if (resp == AVERROR_EOF) {
            cout << "Reached end of file" << endl;
            goto clearFrames;
        } else if (resp < 0) {
            cout << "Error while receiving a frame from the decoder: " << av_err2str(resp) << endl;
            // Failed to get a frame from the decoder
            av_frame_unref(pFrame);
            av_freep(pFrame);
            return resp;
        }
        if (printFrameData) {
            cout << "frame number: " << pContext->frame_number
                 << ", Pkt_Size: " << pFrame->pkt_size
                 << ", Pkt_pts: " << pFrame->pts
                 << ", Pkt_keyFrame: " << pFrame->key_frame << endl;

        }
        unsigned char *buf = pFrame->extended_data[0];
        int wrap = pFrame->linesize[0];
        int xSize = pFrame->width;
        int ySize = pFrame->height;


        if (xSize > 0 && ySize > 0) {
            for (int i = 0; i < ySize; i++) {
                fwrite(buf + i * wrap, 1, xSize, outfile);

            }
            cout << "writing to file" << endl;
        } else {
            cout << "warning: empty data" << endl;
        }

//        int data_size = av_get_bytes_per_sample(pContext->sample_fmt);
//        if (data_size < 0) {
//            /* This should not occur, checking just for paranoia */
//            fprintf(stderr, "Failed to calculate data size\n");
//            exit(1);
//        }
//        for (int i = 0; i < pFrame->nb_samples; i++)
//            for (int ch = 0; ch < pContext->channels; ch++)
//                fwrite(pFrame->data[ch] + data_size * i, 1, data_size, outfile);

        //       saveAudioFrame(pFrame, outfile);
    }
//=======================================
    //TODO: found needed link: https://fossies.org/linux/ffmpeg/doc/examples/demuxing_decoding.c
    //=========================================

    //saveAudioFrame(pFrame, outfile);
    //TODO: only focussing on reading the files


    return 0;
}

void saveAudioFrame(AVFrame *pFrame, FILE *outFile) {


    unsigned char *buf = pFrame->data[0];
    int wrap = pFrame->linesize[0];
    int xSize = pFrame->width;
    int ySize = pFrame->height;

    if (xSize > 0 && ySize > 0) {
        for (int i = 0; i < ySize; i++) {
            fwrite(buf + i * wrap, 1, xSize, outFile);
            cout << "writing to file" << endl;
        }
    } else {
        cout << "warning: empty data" << endl;
    }
}

void openFiles() {
    inFile = fopen(inputFP, "rb");
    outFile = fopen(outputFP, "wb");
    if (inFile == nullptr || outFile == nullptr) {
        cout << stderr << "ERROR: could not open files" << endl;
        fclose(inFile);
        fclose(outFile);
        exit(1);
    }
}

void initializeAllObjects() {
    //open the file and read header - codecs not opened
    //AVFormatContext (what allocate memory for)
    //url to file
    //AVINputFormat -give Null and it will do auto detect
    //try to get some information of the file vis
    // http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49
    //TODO: check if this has to be !=0, or <=0
    int resp = avformat_open_input(&pFormatContext, inputFP, nullptr, nullptr);
    if (resp != 0) {
        cout << stderr << " ERROR: could not open file: " << av_err2str(resp) << endl;
        exit(1);
    }
    // read Packets from the Format to get stream information
    //if the fine does not have a ehader ,read some frames to figure out the information and storage type of the file
    // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb
    if (avformat_find_stream_info(pFormatContext, nullptr) < 0) {
        cout << stderr << " ERROR could not get the stream info" << endl;
        exit(1);
    }


}

int initCodec(enum AVMediaType mediaType) {
    int ret = av_find_best_stream(pFormatContext, mediaType, -1, -1, nullptr, 0);
    if (ret < 0) {
        cout << "ERROR: Could not find %s stream in input file: " << av_get_media_type_string(mediaType) << ", "
             << inputFP << endl;
        return ret;
    }
    cout << "\tfound audio stream" << endl;

    avStreamIndex = ret;

    AVStream *avStream = pFormatContext->streams[avStreamIndex];

    // finds the registered decoder for a codec ID
    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
    pCodec = avcodec_find_decoder(avStream->codecpar->codec_id);
    if (pCodec == nullptr) {
        cout << stderr << " ERROR unsupported codec: " << av_err2str(AVERROR(EINVAL)) << endl;
        exit(1);
    }
    cout << "\tfound decoder" << endl;
    //HERE
    // https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
    //get the context of the audio pCodec- hold info for encode/decode process
    pCodecContext = avcodec_alloc_context3(pCodec);
    if (!pCodecContext) {
        cout << stderr << " ERROR: Could not allocate audio pCodec context: " << av_err2str(AVERROR(ENOMEM)) << endl;
        exit(1);
    }
    cout << "\tallocated context" << endl;

    // Fill the codec context based on the values from the supplied codec parameters
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
    if (avcodec_parameters_to_context(pCodecContext, avStream->codecpar) < 0) {
        cout << stderr << "failed to copy codec params to codec context";
        exit(1);
    }
    cout << "\tcopied codec param" << endl;
    //open the actual pCodec:
    if (avcodec_open2(pCodecContext, pCodec, nullptr) < 0) {
        cout << stderr << "Could not open pCodec" << endl;
        exit(1);
    }
    cout << "\topened codec!" << endl;
    return 0;
}
