// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

// Standard library includes
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <curl/curl.h>

// Local includes
#include "helper.hpp"
#include "labels.hpp"


namespace label {

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

void load_label_file(std::vector<std::string> &class_labels, const std::string &labelfile)
{
    util::log_info("Loading label file " + labelfile);
    std::ifstream file(labelfile);

    if (file.is_open())
    {
        class_labels.clear();

        std::string line;
        while (getline(file, line))
        {
            // remove \r in the end of line
            if (!line.empty() && line[line.size() - 1] == '\r')
            {
                line.erase(line.size() - 1);
            }
            class_labels.push_back(line);
        }
        file.close();
    }
    else
    {
        util::log_error("Could not open file " + labelfile);
    }
}

size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string get_ar_label(std::string label) {
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();    
    std::string readBuffer;
    std::string urlquery;
    std::string normalized_label = label;
    std::size_t space_position = label.find(" ");
    std::size_t npos = -1;
    if(space_position != npos) {
        util::log_debug("Found space!");
        normalized_label = label.substr(0, space_position) + "%20" + label.substr(space_position + 1);
        util::log_debug("label");
        util::log_debug(label);

        util::log_debug("normalized_label");
        util::log_debug(normalized_label);
        space_position = normalized_label.find(" ");
        while(space_position != npos){
            normalized_label = normalized_label.substr(0, space_position) + "%20" + normalized_label.substr(space_position + 1);
            space_position = normalized_label.find(" ");
        }
    }
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
        util::log_debug(urlquery);            
        /* always cleanup */ 
        curl_easy_cleanup(curl);
        std::size_t pos = readBuffer.find("extract");
        std::string extract = readBuffer;
        if(pos > 0) {
            extract = readBuffer.substr (pos + 10);
        }
        return extract;
  }    
    return "Augmented data for : " + label;
}

} // namespace label
