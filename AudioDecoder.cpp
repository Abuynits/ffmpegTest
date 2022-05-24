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

int AudioDecoder::openInputFile(enum AVMediaType mediaType) {

    int ret = av_find_best_stream(pInFormatContext, mediaType, -1, -1, nullptr, 0);
    if (ret < 0) {
        cerr << "ERROR: Could not find %s stream in input file: " << av_get_media_type_string(mediaType) << ", "
             << inputFP << endl;
        return ret;
    }
    cerr << "\tfound audio stream" << endl;

    avStreamIndex = ret;

    inAudioStream = pInFormatContext->streams[avStreamIndex];

    // finds the registered decoder for a codec ID
    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
    pInCodec = avcodec_find_decoder(inAudioStream->codecpar->codec_id);
    if (pInCodec == nullptr) {
        cerr << stderr << " ERROR unsupported codec: " << av_err2str(AVERROR(EINVAL)) << endl;
        exit(1);
    }
    cerr << "\tfound decoder" << endl;
    //HERE
    // https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
    //get the context of the audio pInCodec- hold info for encode/decode process
    pInCodecContext = avcodec_alloc_context3(pInCodec);
    if (!pInCodecContext) {
        cerr << stderr << " ERROR: Could not allocate audio pInCodec context: " << av_err2str(AVERROR(ENOMEM))
             << endl;
        exit(1);
    }
    cerr << "\tallocated context" << endl;

    // Fill the codec context based on the values from the supplied codec parameters
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
    if (avcodec_parameters_to_context(pInCodecContext, inAudioStream->codecpar) < 0) {
        cerr << stderr << "failed to copy codec params to codec context";
        exit(1);
    }
    cerr << "\tcopied codec param" << endl;
    //open the actual pInCodec:
    if (avcodec_open2(pInCodecContext, pInCodec, nullptr) < 0) {
        cerr << stderr << "Could not open pInCodec" << endl;
        exit(1);
    }

    av_dump_format(pInFormatContext, 0, inputFP, 0);
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
        resp = openOutputFile();
        if (resp < 0) {
            exit(1);
        }
    }

    //take either AVMEDIA_TYPE_AUDIO (Default) or AVMEDIA_TYPE_VIDEO
    if (iCodec) {
        resp = openInputFile();
        if (resp < 0) {
            cerr << "error: cannot create codec" << endl;
            exit(1);
        }

        cerr << "\tcreated codec" << endl;
    }

    inAudioStream = pInFormatContext->streams[avStreamIndex];

    //allocate memory for frame from readings
    // https://ffmpeg.org/doxygen/trunk/structAVPacket.html
    pInFrame = av_frame_alloc();
    if (!pInFrame) {
        cerr << stderr << "Could not open frame" << endl;
        exit(1);
    }
    pOutFrame = av_frame_alloc();
    if (!pOutFrame) {
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

int AudioDecoder::openOutConverterFile() {
    int resp = avio_open(&outIoContext, outputFP, AVIO_FLAG_WRITE);
    if (resp < 0) {
        cout << "ERROR: could not open output file" << endl;
    }
    //Assiciate output fiele with container format context
    pOutFormatContext->pb = outIoContext;
    //guess output format based on file extension:
    pOutFormatContext->oformat = av_guess_format(nullptr, outputFP, nullptr);
    if (pOutFormatContext->oformat == nullptr) {
        cout << "Error: Could not guess output format from file" << endl;
        exit(1);
    }
    //  av_strlcpy(pOutFormatContext->filename, filename, sizeof((*output_format_context)->filename));
    //TODO: need to determine codec id by file extension name:IMPORTANT +++++++++++++++++
    pOutCodec = avcodec_find_decoder(AV_CODEC_ID_AAC);

    if (pOutCodec == nullptr) {
        cout << "Error: Could not find codec" << endl;
        exit(1);
    }
    outAudioStream = avformat_new_stream(pOutFormatContext, pOutCodec);
    if (outAudioStream == nullptr) {
        cout << "Error: Could not find outAudioStream" << endl;
        exit(1);
    }
    pOutCodecContext = avcodec_alloc_context3(pOutCodec);
    if (!pOutCodecContext) {
        cerr << stderr << " ERROR: Could not allocate audio pOutCodec context: " << av_err2str(AVERROR(ENOMEM))
             << endl;
        exit(1);
    }
    // ad->pInCodecContext->channel_layout = av_get_default_channel_layout(ad->pInCodecContext->channels);
    pOutCodecContext->channels = pInCodecContext->channels;
    pOutCodecContext->channel_layout = av_get_default_channel_layout(pInCodecContext->channels);
    pOutCodecContext->sample_rate = pInCodecContext->sample_rate;
    pOutCodecContext->sample_rate = pInCodecContext->sample_fmt;
    pOutCodecContext->bit_rate = pInCodecContext->bit_rate;
    //some containers like mp4 need global headers to be present: mark encoder so that it works like we want it to

    if (pOutFormatContext->oformat->flags & AVFMT_GLOBALHEADER) {
        pOutFormatContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    resp = avcodec_open2(pOutCodecContext, pOutCodec, nullptr);
    if (resp < 0) {
        cerr << stderr << " ERROR: Could not open codec and its context " << endl;
        exit(1);
    }

    return resp;

}

int AudioDecoder::openOutputFile() {
    int resp;

    resp = avformat_alloc_output_context2(&pOutFormatContext, nullptr, nullptr, outputFP);
    if (resp < 0) {
        cerr << "error: cannot allocate output context" << endl;
        return -1;
    }
    streamMappingSize = pInFormatContext->nb_streams;
    streamMapping = new int[streamMappingSize];

    //get the output format for this specific audio stream
    // outputFormat = av_guess_format(nullptr, outputFP, nullptr);
    // pOutFormatContext->oformat = outputFormat;

    if (!streamMapping) {
        cerr << "Error: cannot get stream map" << endl;
        return -1;
    }

    cout << "Stream mapping size: " << streamMappingSize << endl;
    for (int i = 0; i < streamMappingSize; i++) {

        outStream = avformat_new_stream(pOutFormatContext, nullptr);
        if (!outStream) {
            cerr << "Error: unable to allocate output stream" << endl;
            return -1;
        }

        inStream = pInFormatContext->streams[i];
        AVCodecParameters *inCodecpar = inStream->codecpar;

        if (inCodecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            inCodecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            inCodecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            streamMapping[i] = -1;
            continue;
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

    avcodec_free_context(&pInCodecContext);
    av_frame_free(&pInFrame);
    av_packet_free(&pPacket);

}


int AudioDecoder::saveAudioFrame(bool showFrameData) {
    char *time = av_ts2timestr(pInFrame->pts, &pInCodecContext->time_base);
    if (startWriting != 0) {
        startFrame = pInCodecContext->frame_number;
        startWriting++;
    }
    endFrame = pInCodecContext->frame_number;

    size_t lineSize = pInFrame->nb_samples * av_get_bytes_per_sample(pInCodecContext->sample_fmt);
    if (showFrameData)
        printf("audio_frame n:%d nb_samples:%d pts:%s\n",
               audioFrameCount++, pInFrame->nb_samples,
               time);

    fwrite(pInFrame->extended_data[0], 1, lineSize, outFile);
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
    enum AVSampleFormat sampleFormat = pInCodecContext->sample_fmt;
    int channelNum = pInCodecContext->channels;
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
    cerr << "ffplay -f " << sFormat << " -ac " << channelNum << " -ar " << pInCodecContext->sample_rate << " "
         << outputFP
         << endl;

    return 0;
}

int AudioDecoder::initFifo() {
    avBuffer = av_audio_fifo_alloc(pOutCodecContext->sample_fmt, pOutCodecContext->channels, 1);
    if(avBuffer== nullptr) {
        cout << "ERROR: allocated av fifo stack" << endl;
        exit(1);
    }

}

