//
// Created by Alexiy Buynitsky on 4/29/22.
//

#include "AudioDecoder.h"

AudioDecoder::AudioDecoder(const char *inFilePath, const char *outFilePath, bool initCodecs, bool initDemuxer) {

    this->inputFP = inFilePath;
    this->outputFP = outFilePath;
    this->iCodec = initCodecs;
    this->iDemuxer = initDemuxer;



}


void AudioDecoder::openFiles() {
    inFile = fopen(inputFP, "rb");
    outFile = fopen(outputFP, "wb");
    if (inFile == nullptr || outFile == nullptr) {
        cerr << stderr << "ERROR: could not open files" << endl;
        fclose(inFile);
        fclose(outFile);
        exit(1);
    }
}

int AudioDecoder::initCodec(enum AVMediaType mediaType) {

    int ret = av_find_best_stream(pInFormatContext, mediaType, -1, -1, nullptr, 0);
    if (ret < 0) {
        cerr << "ERROR: Could not find %s stream in input file: " << av_get_media_type_string(mediaType) << ", "
             << inputFP << endl;
        return ret;
    }
    cerr << "\tfound audio stream" << endl;

    avStreamIndex = ret;

    audioStream = pInFormatContext->streams[avStreamIndex];

    // finds the registered decoder for a codec ID
    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
    pCodec = avcodec_find_decoder(audioStream->codecpar->codec_id);
    if (pCodec == nullptr) {
        cerr << stderr << " ERROR unsupported codec: " << av_err2str(AVERROR(EINVAL)) << endl;
        exit(1);
    }
    cerr << "\tfound decoder" << endl;
    //HERE
    // https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
    //get the context of the audio pCodec- hold info for encode/decode process
    pCodecContext = avcodec_alloc_context3(pCodec);
    if (!pCodecContext) {
        cerr << stderr << " ERROR: Could not allocate audio pCodec context: " << av_err2str(AVERROR(ENOMEM))
             << endl;
        exit(1);
    }
    cerr << "\tallocated context" << endl;

    // Fill the codec context based on the values from the supplied codec parameters
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
    if (avcodec_parameters_to_context(pCodecContext, audioStream->codecpar) < 0) {
        cerr << stderr << "failed to copy codec params to codec context";
        exit(1);
    }
    cerr << "\tcopied codec param" << endl;
    //open the actual pCodec:
    if (avcodec_open2(pCodecContext, pCodec, nullptr) < 0) {
        cerr << stderr << "Could not open pCodec" << endl;
        exit(1);
    }
    cerr << "\topened codec!" << endl;
    return 0;
}

void AudioDecoder::initializeAllObjects() {
    //open the file and read header - codecs not opened
    //AVFormatContext (what allocate memory for)
    //url to file
    //AVINputFormat -give Null and it will do auto detect
    //try to get some information of the file vis
    // http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49
    pInFormatContext = avformat_alloc_context();
    pOutFormatContext = avformat_alloc_context();

    int resp = avformat_open_input(&pInFormatContext, inputFP, nullptr, nullptr);
    if (resp != 0) {
        cerr << stderr << " ERROR: could not open file: " << av_err2str(resp) << endl;
        exit(1);
    }

    // read Packets from the Format to get stream information
    //if the fine does not have a ehader ,read some frames to figure out the information and storage type of the file
    // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb
    if (avformat_find_stream_info(pInFormatContext, nullptr) < 0) {
        cerr << stderr << " ERROR could not get the stream info" << endl;
        exit(1);
    }
    outputFormat = av_guess_format(nullptr, outputFP, nullptr);
    pInFormatContext->oformat = outputFormat;
    //TODO: need to transfer parameters from input Format context to output
    if (iDemuxer) {
        resp = initDemuxer();
        if (resp < 0) {
            exit(1);
        }
    }

    //take either AVMEDIA_TYPE_AUDIO (Default) or AVMEDIA_TYPE_VIDEO
    if (iCodec) {
        resp = initCodec();
        if (resp < 0) {
            cerr << "error: cannot create codec" << endl;
            exit(1);
        }

        cerr << "\tcreated codec" << endl;
    }

    audioStream = pInFormatContext->streams[avStreamIndex];

    //allocate memory for frame from readings
    // https://ffmpeg.org/doxygen/trunk/structAVPacket.html
    pFrame = av_frame_alloc();
    if (!pFrame) {
        cerr << stderr << "Could not open frame" << endl;
        exit(1);
    }
    //allocate memory for packet and frame readings
    // https://ffmpeg.org/doxygen/trunk/structAVFrame.html
    pPacket = av_packet_alloc();
    if (!pPacket) {
        cerr << stderr << "Could not open packet" << endl;
        exit(1);
    }

}

int AudioDecoder::initDemuxer() {
    int resp;
    //dump input information to stderr
    av_dump_format(pInFormatContext, 0, inputFP, 0);

    resp = avformat_alloc_output_context2(&pOutFormatContext, nullptr, nullptr, outputFP);
    if (resp < 0) {
        cerr << "error: cannot allocate output context" << endl;
        return -1;
    }
    streamMappingSize = pInFormatContext->nb_streams;
    streamMapping = new int[streamMappingSize];

    //get the output format for this specific audio stream
    // outputFormat = av_guess_format(nullptr, outputFP, nullptr);
    pOutFormatContext->oformat = outputFormat;

    if (!streamMapping) {
        cerr << "Error: cannot get stream map" << endl;
        return -1;
    }


    for (int i = 0; i < pInFormatContext->nb_streams; i++) {

        inStream = pInFormatContext->streams[i];
        AVCodecParameters *inCodecpar = inStream->codecpar;

        if (inCodecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            inCodecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            inCodecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            streamMapping[i] = -1;
            continue;
        }

        streamMapping[i] = demuxerStreamIndex++;

        outStream = avformat_new_stream(pOutFormatContext, nullptr);
        if (!outStream) {
            cerr << "Error: unable to allocate output stream" << endl;
            return -1;
        }

        resp = avcodec_parameters_copy(outStream->codecpar, inCodecpar);
        if (resp < 0) {
            cerr << "Error: failed to copy codec parameters" << endl;
            return -1;
        }
        outStream->codecpar->codec_tag = 0;
    }

    av_dump_format(pOutFormatContext, 0, outputFP, 1);

    if (!(pOutFormatContext->flags & AVFMT_NOFILE)) {
        resp = avio_open(&pOutFormatContext->pb, outputFP, AVIO_FLAG_WRITE);
        if (resp < 0) {
            fprintf(stderr, "Could not open output file '%s'", outputFP);
            return -1;
        }
    }

    return 0;
}


void AudioDecoder::closeAllObjects() {
    if (iDemuxer) {
        if (pOutFormatContext && !(pOutFormatContext->flags & AVFMT_NOFILE)) {
            avio_closep(&pOutFormatContext->pb);
        }
    }

    fclose(inFile);
    fclose(outFile);
    avformat_free_context(pOutFormatContext);
    avformat_free_context(pInFormatContext);

    avcodec_free_context(&pCodecContext);
    av_frame_free(&pFrame);
    av_packet_free(&pPacket);

}


int AudioDecoder::saveAudioFrame(bool showFrameData) {
    char *time = av_ts2timestr(pFrame->pts, &pCodecContext->time_base);
    if (startWriting != 0) {
        startFrame = pCodecContext->frame_number;
        startWriting++;
    }
    endFrame = pCodecContext->frame_number;

    size_t lineSize = pFrame->nb_samples * av_get_bytes_per_sample(pCodecContext->sample_fmt);
    if (showFrameData)
        printf("audio_frame n:%d nb_samples:%d pts:%s\n",
               audioFrameCount++, pFrame->nb_samples,
               time);

    fwrite(pFrame->extended_data[0], 1, lineSize, outFile);
}

int AudioDecoder::getSampleFmtFormat(const char **fmt, enum AVSampleFormat audioFormat) {

    int i;
    struct sampleFmtEntry {
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
        struct sampleFmtEntry *entry = &fmtEntries[i];
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

int AudioDecoder::getAudioRunCommand() {
    enum AVSampleFormat sampleFormat = pCodecContext->sample_fmt;
    int channelNum = pCodecContext->channels;
    const char *sFormat;

    if (av_sample_fmt_is_planar(sampleFormat)) {
        const char *packed = av_get_sample_fmt_name(sampleFormat);
        printf("Warning: the sample format the decoder produced is planar "
               "(%s). This example will output the first channel only.\n",
               packed ? packed : "?");
        sampleFormat = av_get_packed_sample_fmt(sampleFormat);
        channelNum = 1;
    }

    if (AudioDecoder::getSampleFmtFormat(&sFormat, sampleFormat) < 0) {
        return -1;
    }
    cerr << "Play the data output File w/" << endl;
    cerr << "ffplay -f " << sFormat << " -ac " << channelNum << " -ar " << pCodecContext->sample_rate << " " << outputFP
         << endl;

    return 0;
}