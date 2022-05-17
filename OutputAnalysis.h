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
    OutputAnalysis(const char *fp);

    void getRMS();

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
