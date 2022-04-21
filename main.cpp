#include <iostream>

#include <libavformat/avformat.h>

using namespace std;

int main() {
    AVFormatContext *pFormatContext = avformat_alloc_context();

    cout << "Hello, World!" << endl;
    return 0;
}
