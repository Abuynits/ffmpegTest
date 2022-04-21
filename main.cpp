#include <iostream>

#include <libavformat/avformat.h>

void openWavFiles(const char *fpIn, const char *fpOut, FILE *fileIn, FILE *fileOut);

using namespace std;

int main() {
    const char *inputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/inputRecording.wav";
    const char *outputFP = "/Users/abuynits/CLionProjects/ffmpegTest5/Recordings/outputRecording.wav";
    FILE *inFile = nullptr, *outFile = nullptr;

    openWavFiles(inputFP, outputFP, inFile, outFile);



    AVFormatContext *pFormatContext = avformat_alloc_context();

    cout << "Hello, World!" << endl;
    return 0;
}

void openWavFiles(const char *fpIn, const char *fpOut, FILE *fileIn, FILE *fileOut) {
    fileIn = fopen(fpIn, "rb");
    fileOut = fopen(fpOut, "wb");

    if (fileIn == nullptr || fileOut == nullptr) {
        cout << "ERROR: could not open files" << endl;
        fclose(fileIn);
        fclose(fileOut);
        exit(1);
    }
}
