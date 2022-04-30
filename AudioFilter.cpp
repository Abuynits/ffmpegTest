//
// Created by Alexiy Buynitsky on 4/29/22.
//

#include "AudioFilter.h"

int AudioFilter::initializeAllObjets() {
    //filtergraph houses all of our filters that we will use
    filterGraph = avfilter_graph_alloc();
    if (filterGraph== nullptr) {
        cout<<"ERROR: Unable to create filterGraph"<<endl;
        return AVERROR(ENOMEM);
    }



}
