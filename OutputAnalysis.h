//
// Created by Alexiy Buynitsky on 5/17/22.
//

#ifndef FFMPEGTEST5_OUTPUTANALYSIS_H
#define FFMPEGTEST5_OUTPUTANALYSIS_H


#include <cstdio>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

class OutputAnalysis {
public:
    /**
     * creats a object to store information
     * sets the stderr output to the provided fp
     * @param fp the fp to which set the stderr
     */
    OutputAnalysis(const char *fp);

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
    //TODO: add more specific methods: percentages of audio removed, other ideas for processing
    void setFrameVals(int startMs, int endMs, int totalMs);

    //store the RMS values for the peak and troughs before and after processing
    double bPeak = 0, bTrough = 0, aPeak = 0, aTrough = 0;
    //store the frame # of the start clipped audio, the end clip audio, and the total frames of the original audio
    int startMs, endMs, totalMs;
private:
    const char *filePath;
    //the keys for which to look for in the stderr output-> do not change unless ffmpeg changes these keys
    const string rmsTrough = "RMS trough dB: ";
    const string rmsPeak = "RMS peak dB: ";
    //the file which is opened to read over the stderr output
    ifstream f;

    /**
     * loops over character by character and checks whether there is a sequence of characters in keys that matches lines
     * then it gets the string of the remaining line, and sets it to the value,
     * depending on whether it is the before or after loop
     * @param line the line from the file
     * @param key the key from the file: either rmsTrough or rmsPeak
     */
    void checkForData(string line, string key);


};


#endif //FFMPEGTEST5_OUTPUTANALYSIS_H
