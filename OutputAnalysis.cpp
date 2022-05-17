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

void OutputAnalysis::setFrameVals(int startFrame, int endFrame, int totalFrame) {
    this->startFrame = startFrame;
    this->endFrame = endFrame;
    this->totalFrame = totalFrame;
}

OutputAnalysis::OutputAnalysis(const char *fp) {
    filePath = fp;
    freopen(filePath, "w", stderr);
    f.open(fp);
    cerr << "opened stat file!" << endl;
}
