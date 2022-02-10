// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

// Standard library includes
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <curl/curl.h>
#include <iostream>

// Third party includes
#include <opencv2/gapi/mx.hpp>
#include <opencv2/gapi/core.hpp>
#include <opencv2/gapi/infer.hpp>
#include <opencv2/gapi/streaming/desync.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
// Local includes
#include "helper.hpp"
#include "ar.hpp"


namespace ar {

/* Colors to be used for bounding boxes, etc. */
static const std::vector<cv::Scalar> the_colors = {
        cv::Scalar(0, 0, 255), cv::Scalar(85, 0, 255), cv::Scalar(170, 0, 255),
        cv::Scalar(255, 0, 255), cv::Scalar(255, 0, 170), cv::Scalar(255, 0, 85),
        cv::Scalar(0, 255, 0), cv::Scalar(0, 255, 85), cv::Scalar(0, 255, 170),
        cv::Scalar(0, 255, 255), cv::Scalar(0, 170, 255), cv::Scalar(0, 85, 255),
        cv::Scalar(255, 0, 0), cv::Scalar(255, 85, 0), cv::Scalar(255, 170, 0),
        cv::Scalar(255, 255, 0), cv::Scalar(170, 255, 0), cv::Scalar(85, 255, 0)
};

const std::vector<cv::Scalar>& colors()
{
    return the_colors;
}

size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

size_t write_data(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    std::vector<uchar> *stream = (std::vector<uchar>*)userdata;
    size_t count = size * nmemb;
    stream->insert(stream->end(), ptr, ptr + count);
    return count;
}

std::string get_ar_image(std::string label) {
    //cv::Mat imageFromURL = curlImg("https://upload.wikimedia.org/wikipedia/commons/d/dd/Sofa.jpg", 10);
    std::vector<uchar> stream;
    const char *img_url  = "https://upload.wikimedia.org/wikipedia/commons/d/dd/Sofa.jpg";
    int timeout  = 10;
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, img_url); //the img url
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ar::write_data); // pass the writefunction
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stream); // pass the stream ptr to the writefunction
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout); // timeout if curl_easy hangs, 
    CURLcode res = curl_easy_perform(curl); // start curl

    /* Check for errors */ 
    if(res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
        curl_easy_strerror(res));

    util::log_debug("CURLcode res");
    curl_easy_cleanup(curl); // cleanup    
    cv::Mat imageFromURL = cv::imdecode(stream, -1);
    util::log_debug("got imageFromURL");        

    if (imageFromURL.empty())
        return ""; // load fail

    cv::Rect rect(10, 10, 500, 225);
    cv::rectangle(imageFromURL, rect, cv::Scalar(110, 110, 110), -1);
    return "Image for : " + label;
}

} // namespace label
