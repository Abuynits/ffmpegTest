//
// Created by Alexiy Buynitsky on 4/29/22.
//

#include "AudioDecoder.h"

AudioDecoder::AudioDecoder(const char *inFilePath, const char *outFilePath) {

    this->inputFP = inFilePath;
    this->outputFP = outFilePath;

}


void AudioDecoder::openFiles() {
    inFile = fopen(inputFP, "rb");
    outFile = fopen(outputFP, "wb");
    if (inFile == nullptr || outFile == nullptr) {
        cout << stderr << "ERROR: could not open files" << endl;
        fclose(inFile);
        fclose(outFile);
        exit(1);
    }
}

int AudioDecoder::initCodec(enum AVMediaType mediaType) {

    int ret = av_find_best_stream(pFormatContext, mediaType, -1, -1, nullptr, 0);
    if (ret < 0) {
        cout << "ERROR: Could not find %s stream in input file: " << av_get_media_type_string(mediaType) << ", "
             << inputFP << endl;
        return ret;
    }
    cout << "\tfound audio stream" << endl;

    avStreamIndex = ret;

    audioStream = pFormatContext->streams[avStreamIndex];

    // finds the registered decoder for a codec ID
    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
    pCodec = avcodec_find_decoder(audioStream->codecpar->codec_id);
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
        cout << stderr << " ERROR: Could not allocate audio pCodec context: " << av_err2str(AVERROR(ENOMEM))
             << endl;
        exit(1);
    }
    cout << "\tallocated context" << endl;

    // Fill the codec context based on the values from the supplied codec parameters
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
    if (avcodec_parameters_to_context(pCodecContext, audioStream->codecpar) < 0) {
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

void AudioDecoder::initializeAllObjects() {
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

    //take either AVMEDIA_TYPE_AUDIO (Default) or AVMEDIA_TYPE_VIDEO
    resp = initCodec();
    if (resp == 0) {
        cout << "\tcreated codec" << endl;
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
}

void AudioDecoder::closeAllObjects() {

    fclose(inFile);
    fclose(outFile);

    avcodec_free_context(&pCodecContext);
    av_frame_free(&pFrame);
    av_packet_free(&pPacket);
    cout << "successfully exited program!" << endl;
}


int AudioDecoder::saveAudioFrame() {

    size_t lineSize = pFrame->nb_samples * av_get_bytes_per_sample(pCodecContext->sample_fmt);
    printf("audio_frame n:%d nb_samples:%d pts:%s\n",
           audioFrameCount++, pFrame->nb_samples,
           av_ts2timestr(pFrame->pts, &pCodecContext->time_base));
    fwrite(pFrame->extended_data[0], 1, lineSize, outFile);
}

int AudioDecoder::get_format_from_sample_fmt(const char **fmt, enum AVSampleFormat audioFormat) {

    int i;
    struct sample_fmt_entry {
        enum AVSampleFormat sample_fmt;
        const char *fmt_be, *fmt_le;
    }
            fmtEntries[] = {
            {AV_SAMPLE_FMT_U8,  "u8",    "u8"},
            {AV_SAMPLE_FMT_S16, "s16be", "s16le"},
            {AV_SAMPLE_FMT_S32, "s32be", "s32le"},
            {AV_SAMPLE_FMT_FLT, "f32be", "f32le"},
            {AV_SAMPLE_FMT_DBL, "f64be", "f64le"},
    };

    *fmt = nullptr;

    for (i = 0; i < FF_ARRAY_ELEMS(fmtEntries); i++) {
        struct sample_fmt_entry *entry = &fmtEntries[i];
        if (audioFormat == entry->sample_fmt) {
            *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
            return 0;
        }
    }

    fprintf(stderr,
            "sample format %s is not supported as output format\n",
            av_get_sample_fmt_name(audioFormat));
    return -1;
}
