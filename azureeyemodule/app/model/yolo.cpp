// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

// Standard library includes
#include <string>
#include <thread>
#include <vector>

// Third party includes
#include <opencv2/gapi/mx.hpp>
#include <opencv2/gapi/core.hpp>
#include <opencv2/gapi/infer.hpp>
#include <opencv2/gapi/streaming/desync.hpp>
#include <opencv2/highgui.hpp>

// Local includes
#include "objectdetector.hpp"
#include "yolo.hpp"
#include "../kernels/yolo_kernels.hpp"
#include "../util/helper.hpp"
#include "../util/json.hpp"
#include "../util/labels.hpp"

namespace model {

/** A YOLO network takes a single input and outputs a single output (which we will parse into boxes, labels, and confidences) */
G_API_NET(YOLONetwork, <cv::GMat(cv::GMat)>, "yolo-network");

static const double DEFAULT_CONFIDENCE_THRESHOLD = 0.5;
static const double DEFAULT_NMS_THRESHOLD = 0.5;

YoloModel::YoloModel(const std::string &labelfpath, const std::vector<std::string> &modelfpaths, const std::string &mvcmd,
                    const std::string &videofile, const cv::gapi::mx::Camera::Mode &resolution, const std::string &json_configuration)
    : ObjectDetector{ labelfpath, modelfpaths, mvcmd, videofile, resolution }
{
    this->confidence_threshold = json::parse_string(json_configuration, "YoloConfidenceThreshold", DEFAULT_CONFIDENCE_THRESHOLD);
    this->nms_threshold = json::parse_string(json_configuration, "YoloNMSThreshold", DEFAULT_NMS_THRESHOLD);
}

void YoloModel::run(cv::GStreamingCompiled* pipeline)
{
    while (true)
    {
        this->wait_for_device();

        // Read in the labels for classification
        label::load_label_file(this->class_labels, this->labelfpath);
        this->log_parameters();

        // Build the camera pipeline with G-API
        *pipeline = this->compile_cv_graph();
        util::log_info("starting the pipeline...");
        pipeline->start();

        // Pull data through the pipeline
        bool ran_out_naturally = this->pull_data(*pipeline);

        if (!ran_out_naturally)
        {
            break;
        }
    }
}

cv::GStreamingCompiled YoloModel::compile_cv_graph() const
{
    // Declare an empty GMat - the beginning of the pipeline
    cv::GMat in;
    cv::GMat preproc = cv::gapi::mx::preproc(in, this->resolution);
    cv::GArray<uint8_t> h264;
    cv::GOpaque<int64_t> h264_seqno;
    cv::GOpaque<int64_t> h264_ts;
    std::tie(h264, h264_seqno, h264_ts) = cv::gapi::streaming::encH264ts(preproc);

    // We have BGR output and H264 output in the same graph.
    // In this case, BGR always must be desynchronized from the main path
    // to avoid internal queue overflow (FW reports this data to us via
    // separate channels)
    // copy() is required only to maintain the graph contracts
    // (there must be an operation following desync()). No real copy happens
    cv::GMat img = cv::gapi::copy(cv::gapi::streaming::desync(preproc));

    // This branch has inference and is desynchronized to keep
    // a constant framerate for the encoded stream (above)
    cv::GMat bgr = cv::gapi::streaming::desync(preproc);

    cv::GMat nn = cv::gapi::infer<YOLONetwork>(bgr);

    cv::GOpaque<int64_t> nn_seqno = cv::gapi::streaming::seqNo(nn);
    cv::GOpaque<int64_t> nn_ts = cv::gapi::streaming::timestamp(nn);
    cv::GOpaque<cv::Size> sz = cv::gapi::streaming::size(bgr);

    cv::GArray<cv::Rect> rcs;
    cv::GArray<int> ids;
    cv::GArray<float> cfs;

    std::tie(rcs, ids, cfs) = cv::gapi::streaming::parseYoloWithConf(nn, sz, this->confidence_threshold, this->nms_threshold);

    // Now specify the computation's boundaries
    auto graph = cv::GComputation(cv::GIn(in),
                                  cv::GOut(h264, h264_seqno, h264_ts,      // main path: H264 (~constant framerate)
                                  img,                                     // desynchronized path: BGR
                                  nn_seqno, nn_ts, rcs, ids, cfs, sz));

    auto networks = cv::gapi::networks(cv::gapi::mx::Params<YOLONetwork>{this->modelfiles.at(0)});

    auto kernels = cv::gapi::combine(cv::gapi::mx::kernels(), cv::gapi::kernels<cv::gapi::streaming::GOCVParseYoloWithConf>());

    // Compile the graph in streamnig mode, set all the parameters
    auto pipeline = graph.compileStreaming(cv::gapi::mx::Camera::params(), cv::compile_args(networks, kernels, cv::gapi::mx::mvcmdFile{ this->mvcmd }));

    // Specify the Azure Percept's Camera as the input to the pipeline, and start processing
    pipeline.setSource(cv::gapi::wip::make_src<cv::gapi::mx::Camera>());

    return pipeline;
}

void YoloModel::log_parameters() const
{
    // Log all the stuff
    std::string msg = "blobs: ";
    for (const auto &blob : this->modelfiles)
    {
        msg += blob + ", ";
    }
    msg += ", firmware: " + this->mvcmd + ", parser: YOLO, label: " + this->labelfpath + ", classes: " + std::to_string((int)this->class_labels.size());
    util::log_info(msg);
}

} // namespace model
