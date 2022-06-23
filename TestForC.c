//
// Created by Alexiy Buynitsky on 4/23/22.
//

#include "libavutil/frame.h"
#include "libavutil/mem.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/timestamp.h"
#include "libavutil/samplefmt.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/channel_layout.h"
#include "libavutil/opt.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

/**
 * first loop: loops over the frames present in a packet
 * runs them through the filters and handles eof/ not enough frames
 * @param showFrameData boolean whether to display the info of each frame during output
 * @return whether an error occured
 */
int loopOverPacketFrames(bool showFrameData);

/**
 * adds the frames to the head node of filtergraph
 * recieves the output from the filtergraph
 * writes the raw data to the output file
 * @return whether an error occured
 */
int filterAudioFrame(double dTime);

/**
 * transfers parameters from input codec to output codec
 * estimates the raw data based on the bitrate and other parameters
 * writes the correct header and tail to the final output path
 * transfers the raw data to the output data
 * @param inputPts  tracks the points being looped over
 * @return whether an error occured
 */
int transferStreamData(int *inputPts);

/**
 * first loop: runs all of the frames through the filters
 * writes a header and tail, but they will be fixed in the second loop
 * @return whether an error occured
 */
int applyFilters();

/**
 * the second loop: fixes the headers and tails of the first loop
 * transfers the data as is, but creates custon headers and tails
 * @return whether an error occured
 */
int applyMuxers();

/**
 * check whether input and output file paths are .wav files
 * @param p1 input file path args[1]
 * @param p2 output file path args[2]
 * @return boolean whether they both end in .wav
 */
bool checkIfVaidFilePaths(const char *p1, const char *p2);

/**
 * intialize filter paramters
 * @param numArgs number of filter arguements
 * @param args the parameters
 */
void initFilterParameters(int numArgs, char **args);

/**
 * loops over the stderr output file and looks for key words printed by ffmpeg to find:
 * first;
 * RMS trough dB:
 * RMS peak dB:
 *
 * second:
 * RMS trough dB:
 * RMS peak dB:
 */
void getRMS();

/**
 * set the frame cutoff values after the first filter loop
 * @param startMs the frame where the audio is cut at the start
 * @param endMs the frame where the audio is cut at the end
 * @param totalMs the total frames in the audio
 */
void setFrameVals(int startMs, int endMs, int totalMs);

//store the RMS values for the peak and troughs before and after processing
double bPeak = 0, bTrough = 0, aPeak = 0, aTrough = 0;
//store the frame # of the start clipped audio, the end clip audio, and the total frames of the original audio
int startMs, endMs, totalMs;

/**
  * displays the audio information
  * this includes the start and end frames, the total frame
  * the before and after RMS of the trough and peak of a silent frame
  */
void displayAudioData();

const char *filePath;
//the file which is opened to read over the stderr output
//TODO: commented this out
FILE *f;
//the keys for which to look for in the stderr output-> do not change unless ffmpeg changes these keys
const char *rmsTrough = "RMS trough dB: ";
int rmsTroughLenght = 15;
const char *rmsPeak = "RMS peak dB: ";
int rmsPeakLength = 13;


/**
 * loops over character by character and checks whether there is a sequence of characters in keys that matches lines
 * then it gets the string of the remaining line, and sets it to the value,
 * depending on whether it is the before or after loop
 * @param line the line from the file
 * @param key the key from the file: either rmsTrough or rmsPeak
 */
void checkForData(char *line, int lineSize, char *key, int keySize);

char *startThreshold = "-30dB";
char *startDuration = "0.1";
char *startSilence = "0.1";
char *startPeriod = "1";

char *stopDuration = "2";

//has to be 1: show that want to remove audio
char *stopPeriod = "1";
const char *window = "0.1";
//if close to 0 (ie -10dB), less sensitive
char *stopThreshold = "-40dB";
char *stopSilence = "0";

char *detection = "rms";
char *volume = "1.00";
char *lowPassVal = "10000";
char *highPassVal = "25";

char *audioSampleLength = "0.1";
char *framesResetSample = "2";
char *channelNumber = "0";
char *paramMeasure = "none";

//====loudnorm stats===========
char *iteratedLoudness = "-24.0";
char *loudnessTargetRange = "7.0";
char *maxTruePeak = "-2.0";
char *printFormatType = "none";

//=========== FILTER CONTEXTS and FILTERS here====================
AVFilterGraph *filterGraph = NULL;
//used to pass in the initial frame (the first 'filter' in the graph)
AVFilterContext *srcContext = NULL;
const AVFilter *srcFilter = NULL;


AVFilterContext *lpContext = NULL;
const AVFilter *lpFilter = NULL;

AVFilterContext *hpContext = NULL;
const AVFilter *hpFilter = NULL;

AVFilterContext *arnndnContext = NULL;
const AVFilter *arnndnFilter = NULL;

//silence detector:
AVFilterContext *silenceRemoverContext = NULL;
const AVFilter *silenceRemoverFilter = NULL;

AVFilterContext *beforeStatsContext = NULL;
const AVFilter *beforeStatsFilter = NULL;

AVFilterContext *afterStatsContext = NULL;
const AVFilter *afterStatsFilter = NULL;

//vad filter
AVFilterContext *volumeContext = NULL;
const AVFilter *volumeFilter = NULL;

//the end point of the filter - this is how you specify what to write
AVFilterContext *sinkContext = NULL;
const AVFilter *sinkFilter = NULL;

AVFilterContext *aFormatContext = NULL;
const AVFilter *aFormatFilter = NULL;

AVFilterContext *loudNormFormatContext = NULL;
const AVFilter *loudNormFilter = NULL;

/*
    * creates a filter graph context
    * initilizes each filter with its parameters
    * links the filters together in a specific order:
    * src -> volume -> before Stats -> low pass -> high pass -> arnndn ->
    * silenceremover -> after stats -> aformat -> sink
    * configures the filtegraph
    */
int initializeAllObjets();

/**
 * frees the filtergraph context
 */
void closeAllFilterObjects();

/**
 * displays all of the parameters used in input of audio filter
 */
void showAudioFilterInputs();

/*https://ffmpeg.org/ffmpeg-filters.html#abuffer
 *creates the input filter: used as head node
 * recieves the frames
 */
int initSrcFilter();

/**https://ffmpeg.org/ffmpeg-filters.html#abuffersink
 *creates the output filter: used as tail node
 * outputs the processed frames after they passed thorugh the filtergraph
 * @return whether have an error during init
 */
int initSinkFilter();

/**
 * https://ffmpeg.org/ffmpeg-all.html#loudnorm
 * have a lot of inputs that idk about.
 * intisalizes that audio normalization filter
 * @return whether have an error during init
 */
int initLoudNormFilter();

/**
 *https://ffmpeg.org/ffmpeg-filters.html#volume
 * sets the output volume
 * @return whether have an error during init
 */
int initVolumeFilter();

/**
 *https://ffmpeg.org/ffmpeg-filters.html#lowpass
 * creates the low pass filter
 * @return whether have an error during init
 */
int initLpFilter();

/**
 * https://ffmpeg.org/ffmpeg-filters.html#highpass
 *creates the high pass filter
 * @return whether have an error during init
 */
int initHpFilter();

/**
 *creates the noise remover
 * model selection: beguiling-drafter bc the file is a recording, and we want to remove both noise and non-speech human sounds
 * more info can be found here: https://github.com/GregorR/rnnoise-models
 * need to provide the rnnn node file parameters
 * @return whether have an error during init
 */
int initArnndnFilter();

/**
 *https://ffmpeg.org/ffmpeg-filters.html#aformat-1
 * sets the output format of the filter
 * NOTE: does not specify the format of the demuxer: need 2nd loop to set header and tail after applying filter
 * @return whether have an error during init
 */
int initFormatFilter();

/**
 * https://ffmpeg.org/ffmpeg-filters.html#astat
 * measures the statistics -> mostly the rms during a specific frame
 * important: have parameters as need 2 filters: before and after running the filters
 * same filter, just need 2 different contexts
 * initialized with before stats and after stats
 * NOTE: specify parameters at the top of this file
 * @param afc the specific context of the filter (input context vs output context)
 * @param f the specific filter
 * @return whether have an error during init
 */
int initStatFilter(AVFilterContext **afc, const AVFilter **f);

/**
 *https://ffmpeg.org/ffmpeg-filters.html#silenceremove
 * initializes the silence remove filter
 * runs after the lp and hp filters
 * initializes both removal at the start and removal at the end of an audio file
 * NOTE: specify parameters at the top of this file
 * @return whether have an error during init
 */
int initSilenceRemoverFilter();

/**
 * a general initialization of the filter
 * gets the filter by name
 * allocates the filter to the filtergraph
 * @param af the specific context object of the filter
 * @param f the specifc filter object
 * @param name the name of the filter in the ffmpeg filter
 * @return whether have an error during init
 */
int generalFilterInit(AVFilterContext **af, const AVFilter **f, const char *name);

/**
 * initializes a specific filter parameter with an AVDictionary
 * give it the key and the value, and it specifies them in the specific filter
 * @param afc the specific context for which the parameters should be linked to
 * @param key the varialbe of the parameter
 * @param val the value that the variable should be set to
 * NOTE: check filter documentation for the specifc paramters and the acceptable range of keys
 * NO ERROR CHECK IS DONE - if a value is out of bounds, it will throw an error, but not specify why it was given
 * @return whether have an error during init
 */
static int initByDict(AVFilterContext *afc, const char *key, const char *val);

/**
 * initializes a filter based on specific functions
 * only used to initialize the source filter -> set the metadata (frame rate, bit rate, etc)
 * @param afc
 */
void initByFunctions(AVFilterContext *afc);

char *inputFP = NULL;
char *outputFP = NULL;
FILE *inFile = NULL;
FILE *outFile = NULL;
//input format context: used for input file path
AVFormatContext *pInFormatContext = NULL;
//output format context: used for output file path
AVFormatContext *pOutFormatContext = NULL;
//used to set the specific output format for the input format context when do 1st loop
const AVOutputFormat *outputFormat = NULL;
//the codec used for decoding the input file
const AVCodec *pCodec = NULL;
//the context of the codec: set from the format context and parameters
AVCodecContext *pCodecContext = NULL;
//the packet object used for looping over a audio file
AVPacket *pPacket = NULL;
//the frames of the audio packet taht will be loped over
AVFrame *pFrame = NULL;
//trasnfer the specific input stream between the in and out format context
AVStream *audioStream = NULL;
//the specific index of the audio stream within a packet
int avStreamIndex = -1;
//the count of all audio frames that loop over
int audioFrameCount = 0;
//the array of demuxing information that is accesed with the stream index
int *streamMapping = NULL;
//the specific audio stream index using in demuxing
int demuxerStreamIndex = 0;
//the size of the stream map
int streamMappingSize = 0;
//in and out streams used to transfer information between the in and out codecs
AVStream *outStream = NULL;
AVStream *inStream = NULL;
//set the booleans depending on the function that you want to perform
//first loop = both true, second loop = only demuxer is true
bool iCodec = false;
bool iDemuxer = false;
//track the start and end frames: when start and end writing to the output file
double startTime, endTime;
//start writing when =0: skip the first frame to prevent writing the bad header in the first loop.
int startWriting = -1;

/**
 * init the format contexts
 * get the input format for the input context
 * set the output format for the input context
 * set the input context's stream info
 * set the output formats stream info
 * initCodec and/or initDemuxer is called
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
int saveAudioFrame(bool showFrameData, double time);

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

/**
 * finds the best stream for the codec
 * finds the decoder that corresponds to this stream
 * makes context for the codec
 * sets the context's parameters from the format context
 * opens the codec
 * @param mediaType default = audiotype- the media for which get codec
 * @return whether error happened
 */
int initCodec(enum AVMediaType mediaType);

/**
 * sets a different input parameters to the input format context
 * sets the output format context as an output
 * loops over all of the streams in the input format context
 * formats the corresponding output streams in the output format context
 * opens the streams for writing during looping
 * @return whether error happened
 */
int initDemuxer();

/**
 * reset the variables for second loop
 */
void resetVarsForSecondLoop();

int getLengthOfChar(char *s);

//the input file path
char *usrInputFP;
//stores the raw data after  applying filters
char *usrTempFP;
char *localUsrTempFP = "/fftools/ProcessingFiles/tempRecording.wav";
//the output file path
char *usrFinalFP;
//stores the stderr output which contains rms stats and other debug info
char *statOutFP;
char *localStatOutFP = "/fftools/ProcessingFiles/output.txt";

char *rnnModelPath;
char *localRnnModelPath = "/fftools/ProcessingFiles/rnnoise-models-master/conjoined-burgers-2018-08-28/cb.rnnn";

/**
 * combines the file path from the local directory to the final destination
 * @param currentDir the current working directory
 * @param fileDest the specific file in the current working directory
 * will perform: currentDir/specificFileDestination
 */
char *combineFilePaths(char *currentDir, char *fileDest);

char currentDirectory[PATH_MAX];

double totalTime = 0;
bool showData = false;

int main(int numArgs, char **args) {
    getcwd(currentDirectory, sizeof(currentDirectory));
    // printf("current dir is: %s\n",currentDirectory);
    usrTempFP = combineFilePaths(currentDirectory, localUsrTempFP);
    statOutFP = combineFilePaths(currentDirectory, localStatOutFP);
    rnnModelPath = combineFilePaths(currentDirectory, localRnnModelPath);

    // printf("tempFP: %s\n",usrTempFP);
    const int numFilePaths = 2;
    const int numLenght = 1;

    if (numArgs >= numFilePaths + numLenght) {
        // if (!checkIfVaidFilePaths(args[1], args[2])) {
        //     printf("invalid file type\n");
        //     exit(1);
        // }
        usrInputFP = args[1];
        usrFinalFP = args[2];
        // printf("assuming default processing parameters\n");
    } else {
        usrInputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/inputRecordings/test.wav";
        usrFinalFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/outputRecordings/recordingOut1.wav";

        printf("testing with Alesha's file paths (error if running on server)\n");

    }
    //TODO:IMPORTANT: UNCOMMENT THE 2 LINES BELOW==================
    //f = freopen(statOutFP, "w", stderr);
    //f = fopen(statOutFP, "w");
    //f->open(statOutFP);
    fprintf(stderr, "opened stat file!\n");

    initFilterParameters(numArgs, args);
    //used to error return errors
    int resp;
    //create audio decoder
    inputFP = usrInputFP;
    //TODO:=============IMPORTANT: NEED TO SWITCH OUT HTE COMMENTS IN OUTPUT FILE======
    //outputFP = usrTempFP;
    outputFP = "/Users/abuynits/Desktop/tempRecordingOut.wav";
    iCodec = true;
    iDemuxer = true;
    openFiles();
    initializeAllObjects();

    if (resp < 0) {
        fprintf(stderr, "error: could not initialize decoder\n");
        return 1;
    }
    fprintf(stderr, "initialized AudioDecoder\n");
    //create audio filter


    resp = initializeAllObjets();
    showAudioFilterInputs();
    if (resp < 0) {
        fprintf(stderr, "error: could not initialize filters\n");
        return 1;
    }
    fprintf(stderr, "initialized AudioFilter\n\n");

    //loop over the frames in each packet and apply a chain of filters to each frame
    resp = applyFilters();
    if (resp == 0) {
        fprintf(stderr, "finished processing first loop\n");
    }

    //close all of the objects
    closeAllObjects();

    closeAllFilterObjects();


    //save the stats from the filter stage into the info object
    setFrameVals(startTime, endTime, totalTime * 1000);

    //========================SECOND STAGE: make playable by wav output file==========================

//    resetVarsForSecondLoop();

    //  inputFP = usrTempFP;
    //  outputFP = usrFinalFP;
    // iCodec = false;
    // iDemuxer = true;
    // openFiles();


    //  initializeAllObjects();
    //loop over the file to get the correct header to be playable

    //  resp = applyMuxers();
    //  if (resp == 0) {
    //      fprintf(stderr, "finished muxing files\n");
    //  }

    // closeAllFilterObjects();
    // closeAllObjects();

//    //==============RMS PROCESSING===================
//get the stats and show them

    displayAudioData();
    return 0;
}

char *combineFilePaths(char *currentDir, char *fileDest) {
    int lenDir = getLengthOfChar(currentDir);
    int lenFile = getLengthOfChar(fileDest);
    char *tmpDest = malloc(lenDir + lenFile);
    for (int i = 0; i < lenDir; i++) {
        tmpDest[i] = currentDir[i];
    }
    for (int i = 0; i < lenFile; i++) {
        tmpDest[i + lenDir] = fileDest[i];
    }
    return tmpDest;
}

void resetVarsForSecondLoop() {
    inFile = NULL;
    outFile = NULL;
//input format context: used for input file path
    pInFormatContext = NULL;
//output format context: used for output file path
    pOutFormatContext = NULL;
//used to set the specific output format for the input format context when do 1st loop
    outputFormat = NULL;
//the codec used for decoding the input file
    pCodec = NULL;
//the context of the codec: set from the format context and parameters
    pCodecContext = NULL;
//the packet object used for looping over a audio file
    pPacket = NULL;
//the frames of the audio packet taht will be loped over
    pFrame = NULL;
//trasnfer the specific input stream between the in and out format context
    audioStream = NULL;
//the specific index of the audio stream within a packet
    avStreamIndex = -1;
//the count of all audio frames that loop over
    audioFrameCount = 0;
//the array of demuxing information that is accesed with the stream index
    streamMapping = NULL;
//the specific audio stream index using in demuxing
    demuxerStreamIndex = 0;
//the size of the stream map
    streamMappingSize = 0;
//in and out streams used to transfer information between the in and out codecs
    outStream = NULL;
    inStream = NULL;
}

int transferStreamData(int *inputPts) {
    AVStream *inStream, *outStream;
    inStream = pInFormatContext->streams[pPacket->stream_index];

    if (pPacket->stream_index >= streamMappingSize ||
        streamMapping[pPacket->stream_index] < 0) {
        av_packet_unref(pPacket);
        return 1;
    }

    pPacket->stream_index = streamMapping[pPacket->stream_index];

    outStream = pOutFormatContext->streams[pPacket->stream_index];

    pPacket->duration = av_rescale_q(pPacket->duration, inStream->time_base, outStream->time_base);

    pPacket->pts = *inputPts;
    pPacket->dts = *inputPts;
    *inputPts += pPacket->duration;
    pPacket->pos = -1;
    return 0;
}

int loopOverPacketFrames(bool showFrameData) {
    int resp = avcodec_send_packet(pCodecContext, pPacket);
    if (resp < 0) {
        fprintf(stderr, "error submitting a packet for decoding: ");// << av_err2str(resp);
        return resp;
    }
    while (resp >= 0) {
        resp = avcodec_receive_frame(pCodecContext, pFrame);
        if (resp == AVERROR(EAGAIN)) {
            if (showFrameData) fprintf(stderr, "maNot enough data in frame, skipping to next packet\n");
            //decoded not have enough data to process frame
            //not error unless reached end of the stream - pass more packets untill have enough to produce frame
            clearFrames:
            av_frame_unref(pFrame);
            av_freep(pFrame);
            break;
        } else if (resp == AVERROR_EOF) {
            fprintf(stderr, "Reached end of file\n");
            goto clearFrames;
        } else if (resp < 0) {
            fprintf(stderr, "Error while receiving a frame from the decoder: ");// << av_err2str(resp) << endl;
            // Failed to get a frame from the decoder
            av_frame_unref(pFrame);
            av_freep(pFrame);
            return resp;
        }
        /*
         * TODO: need to find the noise level of audio file
         * try to look at astats filter, then at the portions where silence is detected idk
         * need to get the RMS factor: what kolya talk about
         */

        if (showFrameData)
            printf("frame number: %d, Pkt_Size: %d, Pkt_pts: %lld, Pkt_keyFrame: %d", pCodecContext->frame_number,
                   pFrame->pkt_size, pFrame->pts, pFrame->key_frame);
//            cout << "frame number: " << pCodecContext->frame_number
//                 << ", Pkt_Size: " << pFrame->pkt_size
//                 << ", Pkt_pts: " << pFrame->pts
//                 << ", Pkt_keyFrame: " << pFrame->key_frame << endl;

        char *time = av_ts2timestr(pFrame->pts, &pCodecContext->time_base);

        double dTime = strtod(time, NULL);
        // cout << "time: " << dTime << endl;
        if (filterAudioFrame(dTime) < 0) {
            fprintf(stderr, "error in filtering\n");

        }

        av_frame_unref(pFrame);
        av_freep(pFrame);
        if (resp < 0) {
            return resp;
        }
        totalTime = dTime;
    }
    return 0;

}

int filterAudioFrame(double dTime) {
    //add to source frame:
    int resp = av_buffersrc_add_frame(srcContext, pFrame);
    //TODO:START
    if (resp < 0) {
        fprintf(stderr, "Error: cannot send to graph: ");// << av_err2str(resp) << endl;
        breakFilter:
        av_frame_unref(pFrame);
        return resp;
    }
    //get back the filtered data:
    while ((resp = av_buffersink_get_frame(sinkContext, pFrame)) >= 0) {

        if (resp < 0) {
            fprintf(stderr, "Error filtering data ");// << av_err2str(resp) << endl;
            goto breakFilter;
        }

        saveAudioFrame(showData, dTime * 1000);

        if (resp < 0) {
            fprintf(stderr, "Error muxing packet\n");
        }
        av_frame_unref(pFrame);
    }
    return 0;
}

int applyFilters() {
    int resp = 0;
    resp = avformat_write_header(pOutFormatContext, NULL);
    if (resp < 0) {
        fprintf(stderr, "Error when writing header\n");
        return -1;
    }

    while (av_read_frame(pInFormatContext, pPacket) >= 0) {
        resp = loopOverPacketFrames(showData);
        if (resp < 0) {
            break;
        }
    }

    //flush the audio decoder
    pPacket = NULL;
    loopOverPacketFrames(showData);

    av_write_trailer(pOutFormatContext);


    resp = getAudioRunCommand();
    if (resp < 0) {
        fprintf(stderr, "ERROR getting ffplay command\n");
        return resp;
    }

    return 0;
}

int applyMuxers() {
    int resp = avformat_write_header(pOutFormatContext, NULL);
    if (resp < 0) {
        fprintf(stderr, "Error when opening output file\n");
        return -1;
    }
    int inputPts = 0;
    while (1) {
        resp = av_read_frame(pInFormatContext, pPacket);
        if (resp < 0) {
            break;
        }

        if (transferStreamData(&inputPts) == 1) { continue; }

        resp = av_interleaved_write_frame(pOutFormatContext, pPacket);
        if (resp < 0) {
            fprintf(stderr, "Error muxing packet\n");
            break;
        }
        av_packet_unref(pPacket);
    }
    av_write_trailer(pOutFormatContext);
    return 0;
}

int getLengthOfChar(char *s) {

    int length = 0;
    char c = s[length];
    while (c != '\0') {
        length++;
        c = s[length];
    }
    return length;
}

bool checkIfVaidFilePaths(const char *p1, const char *p2) {

    char *s1 = p1;
    char *s2 = p2;
    int s1Length = getLengthOfChar(s1);
    int s2Length = getLengthOfChar(s2);
    s1 = &s1[s1Length - 4];
    s2 = &s2[s2Length - 4];

    if (strcmp(s1, s2) == 0 && strcmp(s1, ".wav") == 0) {
        return true;
    }

    return false;
}

void initFilterParameters(int numArgs, char **args) {
    const int numFilePaths = 2;
    const int numLenght = 1;
    const int numAllFilterInputs = 16;
    const int numKeyFilterInputs = 6;
    const int numOnlyThresholdInputs = 2;

    if (numArgs == numFilePaths + numLenght) {
        printf("assuming default processing parameters\n");
    } else if (numArgs == numLenght) {
        printf("testing with Alesha's file paths (error if running on server)\n");
    } else if (numArgs == numLenght + numFilePaths + numAllFilterInputs) {

        startThreshold = args[3];
        stopThreshold = args[4];
        window = args[5];
        audioSampleLength = args[6];
        stopDuration = args[7];
        startDuration = args[8];

        stopSilence = args[9];
        startSilence = args[10];
        detection = args[11];
        framesResetSample = args[12];
        channelNumber = args[13];
        startPeriod = args[14];
        stopPeriod = args[15];
        volume = args[16];
        lowPassVal = args[17];
        highPassVal = args[18];

        fprintf(stderr, "specifying all filter parameters\n");

    } else if (numArgs == numLenght + numFilePaths + numKeyFilterInputs) {

        startThreshold = args[3];
        stopThreshold = args[4];
        window = args[5];
        audioSampleLength = args[6];
        stopSilence = args[7];
        startDuration = args[8];
        fprintf(stderr, "specifying key filter parameters\n");
    } else if (numArgs == numLenght + numFilePaths + numOnlyThresholdInputs) {

        startThreshold = args[3];
        stopThreshold = args[4];
        fprintf(stderr, "specifying only threshold parameters\n");
    } else {
        fprintf(stderr, "error: invalid number inputs\n");
        exit(1);
    }
}

void getRMS() {
    //the file which is opened to read over the stderr output
    FILE *statFileReader = fopen(statOutFP, "r");
    // ifstream f(statOutFP);

    char l[256];
    if (statFileReader != NULL) {
        int lineSize;

        while (fgets(l, sizeof(l), statFileReader)) {

            lineSize = getLengthOfChar(l);
            //here, l contains the value of the rms, but need to look for:
            /*

             */
            checkForData(l, lineSize, rmsTrough, rmsTroughLenght);
            checkForData(l, lineSize, rmsPeak, rmsPeakLength);
        }
        //   cout << "Values: " << bPeak << " " << aPeak << " " << aTrough << " " << bTrough << endl;
    }
    fclose(statFileReader);
}

void checkForData(char *line, int lineSize, char *key, int keySize) {
    int i;
    bool match;
    char *temp;
    if (lineSize > keySize) {
        for (i = 0; i < lineSize; i++) {
            match = true;
            for (int j = 0; j < keySize; j++) {
                if (line[i + j] != key[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                i += keySize;
                temp = &line[i];
                //temp = line.substr(i, line.length() - i);
                // cout << "IMPORTANT" << temp << endl;
                if (key == rmsTrough) {
                    if (bTrough == 0) {
                        bTrough = strtod(temp, NULL);
                    } else {
                        aTrough = strtod(temp, NULL);
                    }
                } else if (key == rmsPeak) {
                    if (bPeak == 0) {
                        bPeak = strtod(temp, NULL);
                    } else {
                        aPeak = strtod(temp, NULL);
                    }
                }
            }

        }
    }
}

void setFrameVals(int sMs, int eMs, int tMs) {
    startMs = sMs;
    endMs = eMs;
    totalMs = tMs;
}

void displayAudioData() {

    fflush(stderr);
    fflush(stdout);
//    cerr.flush();
//    cout.flush();

    getRMS();

    printf("===============VIDEO DATA===============\n");
    printf("%d ms removed from start\n", startMs);
    //   cout << startMs << " ms removed from start\n";
    endMs = abs(totalMs - endMs);
    printf("%d ms removed from end\n", endMs);
    float dif = bTrough - aTrough;
    if (dif < 0) {
        dif = -dif;
    }

    printf("Noise removed: %f DB\n", dif);
    //cout << endMs << " ms removed from end\n";

//    printf("before trough rms: %f DB after trough rms: %f DB\n", bTrough, aTrough);

//    cout << "before trough rms: " << bTrough << " DB after trough rms: " << aTrough << " DB"
//         << endl;
    //cout %f "before peak rms: " << bPeak << " DB after peak rms: " << aPeak << " DB\n";
    //  printf("before peak rms: %f DB after peak rms: %f DB\n", bPeak, aPeak);
}

int initializeAllObjets() {
    //filtergraph houses all of the filters that we will use

    filterGraph = avfilter_graph_alloc();
    if (filterGraph == NULL) {
        fprintf(stderr, "ERROR: Unable to create filterGraph\n");
        return AVERROR(ENOMEM);
    }

    //----------------SRC FILTER CREATION----------------
    if (initSrcFilter() < 0) return -1;
    //----------------Volume FILTER CREATION----------------
    if (initVolumeFilter() < 0) return -1;
    //----------------Volume FILTER CREATION----------------
    if (initFormatFilter() < 0) return -1;
    //----------------low pass FILTER CREATION----------------
    if (initLpFilter() < 0) return -1;
    //----------------high pass FILTER CREATION----------------
    if (initHpFilter() < 0) return -1;
    //----------------arnndn FILTER CREATION----------------
    if (initArnndnFilter() < 0) return -1;
    //----------------before stats FILTER CREATION----------------
    if (initStatFilter(&beforeStatsContext, &beforeStatsFilter) < 0) return -1;
    //----------------after stats FILTER CREATION----------------
    if (initStatFilter(&afterStatsContext, &afterStatsFilter) < 0) return -1;
    //----------------silenceremove FILTER CREATION----------------
    if (initSilenceRemoverFilter() < 0) return -1;
    //----------------SINK FILTER CREATION----------------
    if (initSinkFilter() < 0) return -1;
    //----------------NORMALIZATION FILTER CREATION------------
    if (initLoudNormFilter() < 0) return -1;
    /*
     * NOTES:
     * use arnndn filter which uses rnn, give it a model
     * https://github.com/GregorR/rnnoise-models
     */

//------------CONNECT THE FILTERS ------------------
    int resp;
    resp = avfilter_link(srcContext, 0, volumeContext, 0);
    if (resp >= 0) {
        resp = avfilter_link(volumeContext, 0, beforeStatsContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(beforeStatsContext, 0, lpContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(lpContext, 0, hpContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(hpContext, 0, arnndnContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(arnndnContext, 0, loudNormFormatContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(loudNormFormatContext, 0, afterStatsContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(afterStatsContext, 0, silenceRemoverContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(silenceRemoverContext, 0, aFormatContext, 0);
    }
    if (resp >= 0) {
        resp = avfilter_link(aFormatContext, 0, sinkContext, 0);
    }
    if (resp < 0) {
        fprintf(stderr, "Error connecting filters: ");// << av_err2str(resp) << endl;
        return resp;
    }

    resp = avfilter_graph_config(filterGraph, NULL);
    if (resp < 0) {
        fprintf(stderr, "ERROR: cannot configure filter graph ");// << av_err2str(resp) << endl;
        return resp;
    }
    fprintf(stderr, "configured graph!\n");

    return 0;
}

int initSrcFilter() {
    int resp = generalFilterInit(&srcContext, &srcFilter, "abuffer");
    if (resp < 0) {
        return resp;
    }
//check if have channel layout:
    if (!pCodecContext->channel_layout) {
        fprintf(stderr, "\twarning: channel context not initialized... initializing\n");
        pCodecContext->channel_layout = av_get_default_channel_layout(pCodecContext->channels);
    }

    initByFunctions(srcContext);

    resp = avfilter_init_str(srcContext, NULL);
    if (resp < 0) {
        fprintf(stderr, "ERROR: creating srcFilter: ");// << av_err2str(resp) << endl;
        return resp;
    }
    fprintf(stderr, "\tcreated srcFilter!\n");

    return 0;
}

int initSinkFilter() {

    int resp = generalFilterInit(&sinkContext, &sinkFilter, "abuffersink");
    if (resp < 0) {
        return resp;
    }

    resp = avfilter_init_str(sinkContext, NULL);
    if (resp < 0) {
        fprintf(stderr, "ERROR: creating sinkFilter: ");// << av_err2str(resp) << endl;
        return resp;
    }
    fprintf(stderr, "\tCreated sink Filter!\n");
    return 0;
}

int initLoudNormFilter() {
    int resp = generalFilterInit(&loudNormFormatContext, &loudNormFilter, "loudnorm");
    if (resp < 0) {
        fprintf(stderr, "Could not initialize the loudNorm filter.\n");
        return resp;
    }
    resp = initByDict(loudNormFormatContext, "print_format", printFormatType);
    if (resp < 0) {
        fprintf(stderr, "Could not initialize the loudNorm filter.\n");
        return resp;
    }
    resp = initByDict(loudNormFormatContext, "i", iteratedLoudness);
    if (resp < 0) {
        fprintf(stderr, "Could not initialize the loudNorm filter.\n");
        return resp;
    }
    resp = initByDict(loudNormFormatContext, "lra", loudnessTargetRange);
    if (resp < 0) {
        fprintf(stderr, "Could not initialize the loudNorm filter.\n");
        return resp;
    }
    resp = initByDict(loudNormFormatContext, "tp", maxTruePeak);
    if (resp < 0) {
        fprintf(stderr, "Could not initialize the loudNorm filter.\n");
        return resp;
    }

    fprintf(stderr, "\tcreated loudnorm Filter!\n");
    return 0;
}

int initVolumeFilter() {
    int resp = generalFilterInit(&volumeContext, &volumeFilter, "volume");
    if (resp < 0) {
        return resp;
    }
    char *val = AV_STRINGIFY(VOLUME);
    resp = initByDict(volumeContext, "volume", volume);

    if (resp < 0) {
        fprintf(stderr, "Could not initialize the volume filter.\n");
        return resp;
    }

    fprintf(stderr, "\tcreated volume Filter!\n");
    return 0;
}

int initFormatFilter() {
    int resp = generalFilterInit(&aFormatContext, &aFormatFilter, "aformat");
    if (resp < 0) {
        return resp;
    }

    char args[1024];
    snprintf(args, sizeof(args),
             "sample_fmts=%s:sample_rates=%d:channel_layouts=0x%"
                     PRIx64,
             av_get_sample_fmt_name(pCodecContext->sample_fmt), pCodecContext->sample_rate,
             (uint64_t) (pCodecContext->channel_layout));
    resp = avfilter_init_str(aFormatContext, args);
    if (resp < 0) {
        fprintf(stderr, "Could not initialize format filter context: ");// << av_err2str(AV_LOG_ERROR) << endl;
        return resp;
    }
    fprintf(stderr, "\tCreated format Filter!\n");
    return resp;
}

int initLpFilter() {
    int resp = generalFilterInit(&lpContext, &lpFilter, "lowpass");
    if (resp < 0) {
        return resp;
    }
    char *val = AV_STRINGIFY(LOWPASS_VAL);
    resp = initByDict(lpContext, "frequency", lowPassVal);
    fprintf(stderr, "\tCreated lp Filter!\n");
    return resp;
}

int initHpFilter() {
    int resp = generalFilterInit(&hpContext, &hpFilter, "highpass");
    if (resp < 0) {
        return resp;
    }
    char *val = AV_STRINGIFY(HIGHPASS_VAL);
    resp = initByDict(hpContext, "frequency", highPassVal);
    fprintf(stderr, "\tCreated hp Filter!\n");
    return resp;
}

int initSilenceRemoverFilter() {
    int resp = generalFilterInit(&silenceRemoverContext, &silenceRemoverFilter, "silenceremove");
    if (resp < 0) {
        return resp;
    }

//=========GENERAL INIT====================

    char *val = AV_STRINGIFY(START_PERIOD);
    resp = initByDict(silenceRemoverContext, "start_periods", startPeriod);
    if (resp < 0) {
        return resp;
    }
    val = AV_STRINGIFY(STOP_PERIOD);
    resp = initByDict(silenceRemoverContext, "stop_periods", stopPeriod);
    if (resp < 0) {
        return resp;
    }
    val = AV_STRINGIFY(WINDOW);
    resp = initByDict(silenceRemoverContext, "window", window);
    if (resp < 0) {
        return resp;
    }
    val = AV_STRINGIFY(DETECTION);
    resp = initByDict(silenceRemoverContext, "detection", detection);
    if (resp < 0) {
        return resp;
    }
//=========START FILTERS====================


    val = AV_STRINGIFY(START_THRESHOLD);
    resp = initByDict(silenceRemoverContext, "start_threshold", startThreshold);
    if (resp < 0) {
        return resp;
    }

    val = AV_STRINGIFY(START_SILENCE);
    resp = initByDict(silenceRemoverContext, "start_silence", startSilence);
    if (resp < 0) {
        return resp;
    }

//=========END FILTERS====================

    val = AV_STRINGIFY(START_DURATION);
    resp = initByDict(silenceRemoverContext, "start_duration", startDuration);
    if (resp < 0) {
        return resp;
    }
    val = AV_STRINGIFY(STOP_THRESHOLD);
    resp = initByDict(silenceRemoverContext, "stop_threshold", stopThreshold);
    if (resp < 0) {
        return resp;
    }
    //stop_periods=-1:stop_duration=1:stop_threshold=-90dB

    val = AV_STRINGIFY(STOP_SILENCE);
    resp = initByDict(silenceRemoverContext, "stop_silence", stopSilence);
    if (resp < 0) {
        return resp;
    }

    val = AV_STRINGIFY(STOP_DURATION);
    resp = initByDict(silenceRemoverContext, "stop_duration", stopDuration);
    if (resp < 0) {
        return resp;
    }
    fprintf(stderr, "\tCreated silenceremove Filter!\n");
    //use these two values to specify the actual decibal value which is considered silence or not
    //TODO: use https://ffmpeg.org/ffmpeg-filters.html#silencedetect

    return resp;

}

int initArnndnFilter() {
    int resp = generalFilterInit(&arnndnContext, &arnndnFilter, "arnndn");
    if (resp < 0) {
        return resp;
    }

    char *val = rnnModelPath;
    //TODO: IMPORTANT: need to swap these two valus
    //resp = initByDict(arnndnContext, "model", val);
    resp = initByDict(arnndnContext, "model",
                      "/Users/abuynits/CLionProjects/ffmpegTest5/rnnoise-models-master/conjoined-burgers-2018-08-28/cb.rnnn");
    fprintf(stderr, "\tCreated arnndn Filter!\n");
    return resp;
}

void closeAllFilterObjects() {
    avfilter_graph_free(&filterGraph);
}

int initByDict(AVFilterContext *afc, const char *key, const char *val) {

    AVDictionary *optionsDict = NULL;
    av_dict_set(&optionsDict, key, val, 0);

    int resp = avfilter_init_dict(afc, &optionsDict);
    av_dict_free(&optionsDict);

    return resp;
}

void initByFunctions(AVFilterContext *afc) {
    char ch_layout[64];
    av_get_channel_layout_string(ch_layout, sizeof(ch_layout), 0, pCodecContext->channel_layout);
    av_opt_set(afc, "channel_layout", ch_layout, AV_OPT_SEARCH_CHILDREN);
    av_opt_set(afc, "sample_fmt", av_get_sample_fmt_name(pCodecContext->sample_fmt),
               AV_OPT_SEARCH_CHILDREN);
    av_opt_set_q(afc, "time_base", (AVRational) {1, pCodecContext->sample_rate},
                 AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(afc, "sample_rate", pCodecContext->sample_rate, AV_OPT_SEARCH_CHILDREN);
}

int generalFilterInit(AVFilterContext **af, const AVFilter **f, const char *name) {
    *f = avfilter_get_by_name(name);
    if (*f == NULL) {
        fprintf(stderr, "Could not find %s",
                name);
        return AVERROR_FILTER_NOT_FOUND;
    }

    *af = avfilter_graph_alloc_filter(filterGraph, *f, name);
    if (*af == NULL) {
        fprintf(stderr, "Could not allocate %s",
                name);
        return AVERROR(ENOMEM);
    }
    return 0;
}

int initStatFilter(AVFilterContext **afc, const AVFilter **f) {
    int resp = generalFilterInit(afc, f, "astats");
    if (resp < 0) {
        return resp;
    }
    //TODO: need to tune these parameters
    //the number of frames after which recalculate stats
    char *val = AV_STRINGIFY(FRAMES_RESET_SAMPLE);
    resp = initByDict(*afc, "reset", framesResetSample);
    if (resp < 0) {
        return resp;
    }
    //the lenght of audio sample used for determining stats: 0.05 = 5 milliseconds
    val = AV_STRINGIFY(AUDIO_SAMPLE_LENGTH);
    resp = initByDict(*afc, "length", audioSampleLength);
    if (resp < 0) {
        return resp;
    }
    //the channel number = should have 1 as default
    val = AV_STRINGIFY(CHANNEL_NUMBER);
    resp = initByDict(*afc, "metadata", channelNumber);
    if (resp < 0) {
        return resp;
    }
    //not track any parameters
    val = AV_STRINGIFY(PARAM_MEASURE);
    resp = initByDict(*afc, "measure_perchannel", paramMeasure);
    if (resp < 0) {
        return resp;
    }

    return 0;
}

void showAudioFilterInputs() {
    printf("=========filter parameters============\n");
    printf("general filters: \n");
    printf("\tvolume: %s\n", volume);
    printf("\thighpass: %s Hz\n", highPassVal);
    printf("\tlowpass: %s Hz\n", lowPassVal);
    printf("Stat filter: \n");
    printf("\tsample length: %s ms\n", audioSampleLength);
    printf("\ttimes measure: %s\n", framesResetSample);
    printf("\tchannel number: %s\n", channelNumber);
    printf("\tparamters: %s\n", paramMeasure);

    printf("Noise remover: \n");
    printf("\tdetection type: %s\n", detection);
    printf("\twindow: %s sec\n", window);
    printf("\tstart threshold: %s", startThreshold);
    printf("\tstop threshold: %s", stopThreshold);
    printf("\tstart silence duration: %s \n", startSilence);
    printf("\tstop silence duration: %s \n", stopSilence);
    printf("\tstart period: %s \n", startPeriod);
    printf("\tstop period: %s \n", stopPeriod);

    printf("Audio Normalizer: \n");
    printf("\titerated loudness %s \n", iteratedLoudness);
    printf("\tloudness Target Range %s \n", loudnessTargetRange);
    printf("\tmax True peak %s \n", maxTruePeak);
    printf("\tprint Format Type %s \n", printFormatType);

}


void openFiles() {
    inFile = fopen(inputFP, "rb");
    outFile = fopen(outputFP, "wb");
    if (inFile == NULL || outFile == NULL) {
        fprintf(stderr, "ERROR: could not open files\n");
        fclose(inFile);
        fclose(outFile);
        exit(1);
    }
}

int initCodec(enum AVMediaType mediaType) {

    int ret = av_find_best_stream(pInFormatContext, mediaType, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr,
                "ERROR: Could not find %s stream in input file: ", av_get_media_type_string(mediaType));
        // << inputFP << endl;
        return ret;
    }
    fprintf(stderr, "\tfound audio stream\n");

    avStreamIndex = ret;

    audioStream = pInFormatContext->streams[avStreamIndex];

    // finds the registered decoder for a codec ID
    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
    pCodec = avcodec_find_decoder(audioStream->codecpar->codec_id);
    if (pCodec == NULL) {
        fprintf(stderr, " ERROR unsupported codec: %s", av_err2str(AVERROR(EINVAL)));
        //cerr << stderr << " ERROR unsupported codec: " << av_err2str(AVERROR(EINVAL)) << endl;
        exit(1);
    }
    fprintf(stderr, "\tfound decoder\n");
    //HERE
    // https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
    //get the context of the audio pCodec- hold info for encode/decode process
    pCodecContext = avcodec_alloc_context3(pCodec);
    if (!pCodecContext) {
        fprintf(stderr, " ERROR: Could not allocate audio pCodec context: %s", av_err2str(AVERROR(ENOMEM)));
//             << endl;
        exit(1);
    }
    fprintf(stderr, "\tallocated context\n");

    // Fill the codec context based on the values from the supplied codec parameters
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
    if (avcodec_parameters_to_context(pCodecContext, audioStream->codecpar) < 0) {
        fprintf(stderr, "failed to copy codec params to codec context\n");
        exit(1);
    }
    fprintf(stderr, "\tcopied codec param\n");
    //open the actual pCodec:
    if (avcodec_open2(pCodecContext, pCodec, NULL) < 0) {
        fprintf(stderr, "Could not open pCodec\n");
        exit(1);
    }
    fprintf(stderr, "\topened codec!\n");
    return 0;
}

void initializeAllObjects() {
    //open the file and read header - codecs not opened
    //AVFormatContext (what allocate memory for)
    //url to file
    //AVINputFormat -give Null and it will do auto detect
    //try to get some information of the file vis
    // http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49
    pInFormatContext = avformat_alloc_context();
    pOutFormatContext = avformat_alloc_context();

    int resp = avformat_open_input(&pInFormatContext, inputFP, NULL, NULL);
    if (resp != 0) {
        printf(stderr, "ERROR: could not open file: %d\n", av_err2str(resp));
        exit(1);
    }

    // read Packets from the Format to get stream information
    //if the fine does not have a ehader ,read some frames to figure out the information and storage type of the file
    // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb
    if (avformat_find_stream_info(pInFormatContext, NULL) < 0) {
        printf(stderr, " ERROR could not get the stream info\n");
        exit(1);
    }
    outputFormat = av_guess_format(NULL, outputFP, NULL);
    pInFormatContext->oformat = av_guess_format(NULL, outputFP, NULL);
    //TODO: need to transfer parameters from input Format context to output
    if (iDemuxer) {
        resp = initDemuxer();
        if (resp < 0) {
            exit(1);
        }
    }

    //take either AVMEDIA_TYPE_AUDIO (Default) or AVMEDIA_TYPE_VIDEO
    if (iCodec) {
        resp = initCodec(AVMEDIA_TYPE_AUDIO);
        if (resp < 0) {
            fprintf(stderr, "error: cannot create codec\n");
            exit(1);
        }

        fprintf(stderr, "\tcreated codec\n");
    }

    audioStream = pInFormatContext->streams[avStreamIndex];

    //allocate memory for frame from readings
    // https://ffmpeg.org/doxygen/trunk/structAVPacket.html
    pFrame = av_frame_alloc();
    if (!pFrame) {
        printf(stderr, "Could not open frame\n");
        exit(1);
    }
    //allocate memory for packet and frame readings
    // https://ffmpeg.org/doxygen/trunk/structAVFrame.html
    pPacket = av_packet_alloc();
    if (!pPacket) {
        printf(stderr, "Could not open packet\n");
        exit(1);
    }

}

int initDemuxer() {
    int resp;
    //dump input information to stderr
    av_dump_format(pInFormatContext, 0, inputFP, 0);

    resp = avformat_alloc_output_context2(&pOutFormatContext, NULL, NULL, outputFP);
    if (resp < 0) {
        fprintf(stderr, "error: cannot allocate output context\n");
        return -1;
    }
    streamMappingSize = pInFormatContext->nb_streams;
    const int tempLength = pInFormatContext->nb_streams;
    int tempStream[tempLength];
    streamMapping = &(tempStream[0]);
    //get the output format for this specific audio stream
    // outputFormat = av_guess_format(NULL, outputFP, NULL);
    pOutFormatContext->oformat = av_guess_format(NULL, outputFP, NULL);
    if (!streamMapping) {
        fprintf(stderr, "Error: cannot get stream map\n");
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

        outStream = avformat_new_stream(pOutFormatContext, NULL);
        if (!outStream) {
            fprintf(stderr, "Error: unable to allocate output stream\n");
            return -1;
        }

        resp = avcodec_parameters_copy(outStream->codecpar, inCodecpar);
        if (resp < 0) {
            fprintf(stderr, "Error: failed to copy codec parameters\n");
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


void closeAllObjects() {
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


int saveAudioFrame(bool showFrameData, double time) {
    char *saveTime = av_ts2timestr(pFrame->pts, &pCodecContext->time_base);
    if (startWriting != 0) {
        startTime = time;
        startWriting++;
    }
    endTime = strtod(saveTime, NULL) * 1000 + startTime;

    size_t lineSize = pFrame->nb_samples * av_get_bytes_per_sample(pCodecContext->sample_fmt);
    if (showFrameData)
        printf("audio_frame n:%d nb_samples:%d pts:%s\n",
               audioFrameCount++, pFrame->nb_samples,
               saveTime);

    fwrite(pFrame->extended_data[0], 1, lineSize, outFile);
    return 0;
}

int getSampleFmtFormat(const char **fmt, enum AVSampleFormat audioFormat) {

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

    *fmt = NULL;

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

int getAudioRunCommand() {
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

    if (getSampleFmtFormat(&sFormat, sampleFormat) < 0) {
        return -1;
    }
    fprintf(stderr, "Play the data output File w/\n");
    fprintf(stderr, "ffplay -f %s -ac %s -ar %d ", outputFP, sFormat, channelNum, pCodecContext->sample_rate);
//    cerr << "ffplay -f " << sFormat << " -ac " << channelNum << " -ar " << pCodecContext->sample_rate << " " << outputFP
//         << endl;

    return 0;
}
