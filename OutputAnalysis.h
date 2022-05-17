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
     * @param startFrame the frame where the audio is cut at the start
     * @param endFrame the frame where the audio is cut at the end
     * @param totalFrame the total frames in the audio
     */
     //TODO: add more specific methods: percentages of audio removed, other ideas for processing
     //TODO: finish documenting the outputanalysis and the audiodecoder file
    void setFrameVals(int startFrame, int endFrame, int totalFrame);

    double bPeak = 0, bTrough = 0, aPeak = 0, aTrough = 0;
    int startFrame, endFrame, totalFrame;
private:
    const char *filePath;
    const string rmsTrough = "RMS trough dB: ";
    const string rmsPeak = "RMS peak dB: ";

    ifstream f;

    void checkForData(string line, string key);


};


#endif //FFMPEGTEST5_OUTPUTANALYSIS_H
