//
// Created by Alexiy Buynitsky on 5/17/22.
//

#include "OutputAnalysis.h"

void OutputAnalysis::getRMS() {
    string l;
    if (f.is_open()) {
        while (getline(f, l)) {
            //here, l contains the value of the rms, but need to look for:
            /*

             */
            checkForData(l, rmsTrough);
            checkForData(l, rmsPeak);
        }
        //   cout << "Values: " << bPeak << " " << aPeak << " " << aTrough << " " << bTrough << endl;
    }


}

void OutputAnalysis::checkForData(string line, string key) {
    int i;
    bool match;
    string temp;
    if (line.length() > key.length()) {
        for (i = 0; i < line.length(); i++) {
            match = true;
            for (int j = 0; j < key.length(); j++) {
                if (line[i + j] != key[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                i += key.length();
                temp = line.substr(i, line.length() - i);
                // cout << "IMPORTANT" << temp << endl;
                if (key == rmsTrough) {
                    if (bTrough == 0) {
                        bTrough = stod(temp);
                    } else {
                        aTrough = stod(temp);
                    }
                } else if (key == rmsPeak) {
                    if (bPeak == 0) {
                        bPeak = stod(temp);
                    } else {
                        aPeak = stod(temp);
                    }
                }
            }

        }
    }
}

void OutputAnalysis::setFrameVals(int startMs, int endMs, int totalMs) {
    this->startMs = startMs;
    this->endMs = totalMs - endMs;
    this->totalMs = totalMs;
}

OutputAnalysis::OutputAnalysis(const char *fp) {
    filePath = fp;
    freopen(filePath, "w", stderr);
    f.open(fp);
    cerr << "opened stat file!" << endl;
}

void OutputAnalysis::displayAudioData() {
    cerr.flush();
    getRMS();

    cout << "===============VIDEO DATA===============" << endl;
    cout << startMs << " ms removed from start" << endl;
    cout << endMs << " ms removed from end" << endl;
    cout << "before trough rms: " << bTrough << " DB after trough rms: " << aTrough << " DB"
         << endl;
    cout << "before peak rms: " << bPeak << " DB after peak rms: " << aPeak << " DB" << endl;
}