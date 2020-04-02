// Copyright (C) 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdio>
#include <string>

#include "face_inference_results.hpp"

namespace gaze_estimation {

struct ResultsData {
    //double keypoints[110] = {0};
    int keypoints[50] = {-1};
};

class ResultsMarker {
public:
    ResultsMarker(bool showFaceBoundingBox,
                  bool showHeadPoseAxes,
                  bool showLandmarks,
                  bool showGaze);
    void mark(cv::Mat& image, const FaceInferenceResults& faceInferenceResults) const;
    void mark(cv::Mat& image, const FaceInferenceResults& faceInferenceResults, ResultsData& res_data) const;
    void toggle(char key);

private:
    bool showFaceBoundingBox;
    bool showHeadPoseAxes;
    bool showLandmarks;
    bool showGaze;
};

}  // namespace gaze_estimation
