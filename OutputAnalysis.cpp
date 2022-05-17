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
             * first;
             * RMS trough dB:
             * RMS peak dB:
             *
             * second:
             * RMS trough dB:
             * RMS peak dB:
             */
            checkForData(l, rmsTrough);
            checkForData(l, rmsTrough);
            cout<<l<<endl;
        }

    }


}

void OutputAnalysis::checkForData(string line, string key) {
    int i;
    bool match = true;
    for (i = 0; i < line.length(); i++) {

        for (int j = 0; j < key.length(); j++) {
            if (line[i + j] != key[j]) {
                match = false;
                break;
            }
        }

    }
    if (match) {
        i += key.length();
        string temp = line.substr(i,line.length()-i);
        cout<<temp<<endl;
    }

}