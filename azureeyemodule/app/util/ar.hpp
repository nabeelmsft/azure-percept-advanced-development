#pragma once

// Standard library includes
#include <string>
#include <vector>

// Third party includes
#include <opencv2/core.hpp>


namespace ar {

/** Retrieve the color vector. */
const std::vector<cv::Scalar>& colors();

/** Load AR image */
std::string get_ar_image(std::string label);

/** Load AR text */
std::string get_ar_label(std::string label);

} // namespace label
