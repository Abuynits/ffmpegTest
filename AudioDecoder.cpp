//
// Created by Alexiy Buynitsky on 4/29/22.
//

#include "AudioDecoder.h"

AudioDecoder::AudioDecoder(const char *inFilePath, const char *outFilePath) {

    this->inFilePath = inFilePath;
    this->outFilePath = outFilePath;

}

/**
 * creates file objects for the input and output files
 * @param fpIn input filepath
 * @param fpOut output filepath
 * @param fileIn input file object
 * @param fileOut output file object
 */
void AudioDecoder::openFiles(const char *fpIn, const char *fpOut, FILE *fileIn, FILE *fileOut) {
    fileIn = fopen(fpIn, "rb");
    fileOut = fopen(fpOut, "wb");

    if (fileIn == nullptr || fileOut == nullptr) {
        cout << stderr << "ERROR: could not open files" << endl;
        fclose(fileIn);
        fclose(fileOut);
        exit(1);
    }
}

void AudioDecoder::showDataGetCodec(bool printInfo) {

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
    if (printInfo) {
        const char *fileFormat = pFormatContext->iformat->long_name;
        int64_t duration = pFormatContext->duration;

        cout << "format: " << fileFormat << " duration: " << duration << endl;
        cout << "audio_codec_id: " << pCodecParam->codec_id << endl;
    }
//        string ending = path.substr(loc);
//
//        if (ending == ".wav") {
//            param->codec_id = AV_CODEC_ID_GSM_MS;
//        } else if (ending == ".mp3") {
//            param->codec_id = AV_CODEC_ID_MP3;
//        } else {
//            cout << stderr << "ERROR: not found audio ending" << endl;
//            exit(1);
//        }
//    }
//    if (printInfo) {
//        const char *fileFormat = pContext->iformat->long_name;
//        int64_t duration = pContext->duration;
//
//        cout << "format: " << fileFormat << " duration: " << duration << endl;
//        cout << "audio_codec_id: " << param->codec_id << endl;
//    }
//    return param->codec_id;

}

void AudioDecoder::initializeAllObjects() {
    //hold the header information from the format (file)
    // http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html
    pFormatContext = avformat_alloc_context();//alloc information for format of file
    if (!pFormatContext) {
        cout << stderr << "ERROR could not allocate memory for Format Context" << endl;
        exit(1);
    }
    //open the file and read header - codecs not opened
    //AVFormatContext (what allocate memory for)
    //url to file
    //AVINputFormat -give Null and it will do auto detect
    // http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49
    if (avformat_open_input(&pFormatContext, inFilePath, nullptr, nullptr) != 0) {
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

    showDataGetCodec(true);

    openFiles(inFilePath, outFilePath, inFile, outFile);

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
        exit(1);
    }
    //open the actual pCodec:
    if (avcodec_open2(pCodecContext, pCodec, nullptr) < 0) {
        cout << stderr << "Could not open pCodec" << endl;
        exit(1);
    }

    //allocate memory for packet and frame readings
    // https://ffmpeg.org/doxygen/trunk/structAVFrame.html
    pPacket = av_packet_alloc();
    if (!pPacket) {
        cout << stderr << "Could not open packet" << endl;
        exit(1);
    }
    //allocate memory for frame from readings
    // https://ffmpeg.org/doxygen/trunk/structAVPacket.html
    pFrame = av_frame_alloc();
    if (!pFrame) {
        cout << stderr << "Could not open frame" << endl;
        exit(1);
    }
}

void AudioDecoder::closeAllObjects() {
    fclose(inFile);
    fclose(outFile);

    avcodec_free_context(&pCodecContext);
    av_parser_close(pParser);
    av_frame_free(&pFrame);
    av_packet_free(&pPacket);
}
