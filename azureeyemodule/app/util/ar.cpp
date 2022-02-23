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

/** Call back for the curl */
size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

/** Load AR text */
std::string get_ar_label(std::string label) {
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();    
    std::string readBuffer;
    std::string urlquery;
    std::string normalized_label = label;
    
    /**  Code to replace spaces with %20 for http call*/
    std::size_t space_position = label.find(" ");
    std::size_t npos = -1;
    if(space_position != npos) {
        normalized_label = label.substr(0, space_position) + "%20" + label.substr(space_position + 1);
        space_position = normalized_label.find(" ");
        while(space_position != npos){
            normalized_label = normalized_label.substr(0, space_position) + "%20" + normalized_label.substr(space_position + 1);
            space_position = normalized_label.find(" ");
        }
    }

    /**  Code to retrieve AR data. */    
    urlquery = "https://en.wikipedia.org/w/api.php?action=query&prop=extracts&exintro&explaintext&redirects=1&format=json&titles=" + normalized_label;
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, urlquery.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        /* Perform the request, res will get the return code */ 
        res = curl_easy_perform(curl);
        
        /* Check for errors */ 
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
        /* always cleanup */ 
        curl_easy_cleanup(curl);
        std::size_t pos = readBuffer.find("extract");
        std::string extract = readBuffer;
        if(pos > 0) {
            extract = readBuffer.substr (pos + 10);
        }
        return extract;
  }    
    return "";
}

} // namespace ar
