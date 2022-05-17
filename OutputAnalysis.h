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
    OutputAnalysis(const char *fp) {
        filePath = fp;


        f.open (fp);
        cout << "opened stat file!" << endl;
    }
    void getRMS();

private:
    const char *filePath;
    const string rmsTrough="RMS trough dB: ";
    const string rmsPeak ="RMS peak dB: ";
    double bPeak, bTrough, aPeak, aTrough;
    ifstream f;
    void checkForData(string line, string key);


};


#endif //FFMPEGTEST5_OUTPUTANALYSIS_H
