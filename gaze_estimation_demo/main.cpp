// Copyright (C) 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

/**
* \brief The entry point for the Inference Engine gaze_estimation_demo application
* \file gaze_estimation_demo/main.cpp
* \example gaze_estimation_demo/main.cpp
*/
#include <gflags/gflags.h>
#include <functional>
#include <iostream>
#include <fstream>
#include <random>
#include <memory>
#include <chrono>
#include <vector>
#include <string>
#include <utility>
#include <algorithm>
#include <iterator>
#include <map>
#include <sstream>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core.hpp>

#include <inference_engine.hpp>

#include <monitors/presenter.h>
#include <samples/ocv_common.hpp>
#include <samples/slog.hpp>

#include "gaze_estimation_demo.hpp"

#include "face_inference_results.hpp"

#include "face_detector.hpp"

#include "base_estimator.hpp"
#include "head_pose_estimator.hpp"
#include "landmarks_estimator.hpp"
#include "gaze_estimator.hpp"

#include "results_marker.hpp"

#include "exponential_averager.hpp"

#include "utils.hpp"

#include <ie_iextension.h>
#ifdef WITH_EXTENSIONS
#include <ext_list.hpp>
#endif

using namespace InferenceEngine;
using namespace gaze_estimation;

bool ParseAndCheckCommandLine(int argc, char *argv[]) {
    // ---------------------------Parsing and validating input arguments--------------------------------------
    gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);
    if (FLAGS_h) {
        showUsage();
        showAvailableDevices();
        return false;
    }
    slog::info << "Parsing input parameters" << slog::endl;
    if (FLAGS_i.empty())
        throw std::logic_error("Parameter -i is not set");
    if (FLAGS_m.empty())
        throw std::logic_error("Parameter -m is not set");
    if (FLAGS_m_fd.empty())
        throw std::logic_error("Parameter -m_fd is not set");
    if (FLAGS_m_hp.empty())
        throw std::logic_error("Parameter -m_hp is not set");
    if (FLAGS_m_lm.empty())
        throw std::logic_error("Parameter -m_lm is not set");

    return true;
}


int main_ori(int argc, char *argv[]) {
    try {
        std::cout << "InferenceEngine: " << GetInferenceEngineVersion() << std::endl;

        // ------------------------------ Parsing and validating of input arguments --------------------------
        if (!ParseAndCheckCommandLine(argc, argv)) {
            return 0;
        }

        slog::info << "Reading input" << slog::endl;
        cv::VideoCapture cap;

        if (!(FLAGS_i == "cam" ? cap.open(0) : cap.open(FLAGS_i))) {
            throw std::logic_error("Cannot open input file or camera: " + FLAGS_i);
        }

        // Parse camera resolution parameter and set camera resolution
        if (FLAGS_i == "cam" && FLAGS_res != "") {
            auto xPos = FLAGS_res.find("x");
            if (xPos == std::string::npos)
                throw std::runtime_error("Incorrect -res parameter format, please use 'x' to separate width and height");
            int frameWidth, frameHeight;
            std::stringstream widthStream(FLAGS_res.substr(0, xPos));
            widthStream >> frameWidth;
            std::stringstream heightStream(FLAGS_res.substr(xPos + 1));
            heightStream >> frameHeight;
            cap.set(cv::CAP_PROP_FRAME_WIDTH, frameWidth);
            cap.set(cv::CAP_PROP_FRAME_HEIGHT, frameHeight);
        }

        // read input (video) frame
        cv::Mat frame;
        if (!cap.read(frame)) {
            throw std::logic_error("Failed to get frame from cv::VideoCapture");
        }

        bool flipImage = false;
        ResultsMarker resultsMarker(false, false, false, true);

        // Loading Inference Engine
        std::vector<std::pair<std::string, std::string>> cmdOptions = {
            {FLAGS_d, FLAGS_m}, {FLAGS_d_fd, FLAGS_m_fd},
            {FLAGS_d_hp, FLAGS_m_hp}, {FLAGS_d_lm, FLAGS_m_lm}
        };

        InferenceEngine::Core ie;
        initializeIEObject(ie, cmdOptions);

        // Enable per-layer metrics
        if (FLAGS_pc) {
            ie.SetConfig({{PluginConfigParams::KEY_PERF_COUNT, PluginConfigParams::YES}});
        }

        // Set up face detector and estimators
        FaceDetector faceDetector(ie, FLAGS_m_fd, FLAGS_d_fd, FLAGS_t, FLAGS_fd_reshape);
        HeadPoseEstimator headPoseEstimator(ie, FLAGS_m_hp, FLAGS_d_hp);
        LandmarksEstimator landmarksEstimator(ie, FLAGS_m_lm, FLAGS_d_lm);
        GazeEstimator gazeEstimator(ie, FLAGS_m, FLAGS_d);

        // Put pointers to all estimators in an array so that they could be processed uniformly in a loop
        BaseEstimator* estimators[] = {&headPoseEstimator, &landmarksEstimator, &gazeEstimator};

        // Each element of the vector contains inference results on one face
        std::vector<FaceInferenceResults> inferenceResults;

        // Exponential averagers for times
        double smoothingFactor = 0.1;
        ExponentialAverager overallTimeAverager(smoothingFactor, 30.);
        ExponentialAverager inferenceTimeAverager(smoothingFactor, 30.);

        int delay = 1;
        std::string windowName = "Gaze estimation demo";
        double overallTime = 0., inferenceTime = 0.;
        cv::Size graphSize{static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH) / 4), 60};
        Presenter presenter(FLAGS_u, static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT)) - graphSize.height - 10, graphSize);
        auto tIterationBegins = cv::getTickCount();
        do {
            if (flipImage) {
                cv::flip(frame, frame, 1);
            }

            // Infer results
            auto tInferenceBegins = cv::getTickCount();
            auto inferenceResults = faceDetector.detect(frame);
            for (auto& inferenceResult : inferenceResults) {
                for (auto estimator : estimators) {
                    estimator->estimate(frame, inferenceResult);
                }
            }
            auto tInferenceEnds = cv::getTickCount();

            // Measure FPS
            auto tIterationEnds = cv::getTickCount();
            overallTime = (tIterationEnds - tIterationBegins) * 1000. / cv::getTickFrequency();
            overallTimeAverager.updateValue(overallTime);
            tIterationBegins = tIterationEnds;

            inferenceTime = (tInferenceEnds - tInferenceBegins) * 1000. / cv::getTickFrequency();
            inferenceTimeAverager.updateValue(inferenceTime);

            if (FLAGS_pc) {
                faceDetector.printPerformanceCounts();
                for (auto const estimator : estimators) {
                    estimator->printPerformanceCounts();
                }
            }

            if (FLAGS_r) {
                for (auto& inferenceResult : inferenceResults) {
                    std::cout << inferenceResult << std::endl;
                }
            }

            if (FLAGS_no_show) {
                continue;
            }

            presenter.drawGraphs(frame);

            // Display the results
            for (auto const& inferenceResult : inferenceResults) {
                resultsMarker.mark(frame, inferenceResult);
            }
            putTimingInfoOnFrame(frame, overallTimeAverager.getAveragedValue(),
                                 inferenceTimeAverager.getAveragedValue());
            cv::imshow(windowName, frame);

            // Controls the information being displayed while demo runs
            char key = static_cast<char>(cv::waitKey(delay));
            resultsMarker.toggle(key);

            // Press 'Esc' to quit, 'f' to flip the video horizontally
            if (key == 27)
                break;
            else if (key == 'f')
                flipImage = !flipImage;
            else
                presenter.handleKey(key);
        } while (cap.read(frame));
        std::cout << presenter.reportMeans() << '\n';
    }
    catch (const std::exception& error) {
        slog::err << error.what() << slog::endl;
        return 1;
    }
    catch (...) {
        slog::err << "Unknown/internal exception happened." << slog::endl;
        return 1;
    }
    slog::info << "Execution successful" << slog::endl;
    return 0;
}


#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <iostream>
#include <string>
#include <iomanip>
#define PORT 8080

int main(int argc, char *argv[]) {

    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    std::string msg_open = "OPEN";
    std::string msg_infer = "INFR";
    std::string msg_close = "CLSE";
    std::string msg_down = "DOWN";
    std::string msg_fail = "FAIL";
    char rcv_msg[4];
    ResultsData res_data;

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    try {
        std::cout << "InferenceEngine: " << GetInferenceEngineVersion() << std::endl;

        // ------------------------------ Parsing and validating of input arguments --------------------------
        if (!ParseAndCheckCommandLine(argc, argv)) {
            return 0;
        }

        slog::info << "Reading input" << slog::endl;
        cv::VideoCapture cap;

        if (!(FLAGS_i == "cam" ? cap.open(0) : cap.open(FLAGS_i))) {
            throw std::logic_error("Cannot open input file or camera: " + FLAGS_i);
        }

        // Parse camera resolution parameter and set camera resolution
        if (FLAGS_i == "cam" && FLAGS_res != "") {
            auto xPos = FLAGS_res.find("x");
            if (xPos == std::string::npos)
                throw std::runtime_error("Incorrect -res parameter format, please use 'x' to separate width and height");
            int frameWidth, frameHeight;
            std::stringstream widthStream(FLAGS_res.substr(0, xPos));
            widthStream >> frameWidth;
            std::stringstream heightStream(FLAGS_res.substr(xPos + 1));
            heightStream >> frameHeight;
            cap.set(cv::CAP_PROP_FRAME_WIDTH, frameWidth);
            cap.set(cv::CAP_PROP_FRAME_HEIGHT, frameHeight);
        }

        // read input (video) frame
        #if 0
        cv::Mat frame;
        if (!cap.read(frame)) {
            throw std::logic_error("Failed to get frame from cv::VideoCapture");
        }
        #endif

        bool flipImage = false;
        ResultsMarker resultsMarker(false, false, false, true);

        // Loading Inference Engine
        std::vector<std::pair<std::string, std::string>> cmdOptions = {
            {FLAGS_d, FLAGS_m}, {FLAGS_d_fd, FLAGS_m_fd},
            {FLAGS_d_hp, FLAGS_m_hp}, {FLAGS_d_lm, FLAGS_m_lm}
        };

        InferenceEngine::Core ie;
        initializeIEObject(ie, cmdOptions);

        // Enable per-layer metrics
        if (FLAGS_pc) {
            ie.SetConfig({{PluginConfigParams::KEY_PERF_COUNT, PluginConfigParams::YES}});
        }

        // Set up face detector and estimators
        FaceDetector faceDetector(ie, FLAGS_m_fd, FLAGS_d_fd, FLAGS_t, FLAGS_fd_reshape);
        HeadPoseEstimator headPoseEstimator(ie, FLAGS_m_hp, FLAGS_d_hp);
        LandmarksEstimator landmarksEstimator(ie, FLAGS_m_lm, FLAGS_d_lm);
        GazeEstimator gazeEstimator(ie, FLAGS_m, FLAGS_d);

        // Put pointers to all estimators in an array so that they could be processed uniformly in a loop
        BaseEstimator* estimators[] = {&headPoseEstimator, &landmarksEstimator, &gazeEstimator};

        // Each element of the vector contains inference results on one face
        std::vector<FaceInferenceResults> inferenceResults;

        // Exponential averagers for times
        double smoothingFactor = 0.1;
        ExponentialAverager overallTimeAverager(smoothingFactor, 30.);
        ExponentialAverager inferenceTimeAverager(smoothingFactor, 30.);

        int delay = 1;
        std::string windowName = "Gaze estimation demo";
        double overallTime = 0., inferenceTime = 0.;
        cv::Size graphSize{static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH) / 4), 60};
        Presenter presenter(FLAGS_u, static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT)) - graphSize.height - 10, graphSize);
        auto tIterationBegins = cv::getTickCount();

        std::cout << "Waiting Message" << std::endl << std::flush;
        while ((valread = recv(new_socket, rcv_msg, sizeof(rcv_msg), 0)) >0 ) {
            if (!strncmp(rcv_msg, msg_open.c_str(), 4)) {
                std::cout << "OPEN" << std::endl;
                send(new_socket , msg_down.c_str() , msg_down.size() , 0 ); }
            
            
            else if (!strncmp(rcv_msg, msg_infer.c_str(), 4)) {
                std::cout << "INFER" << std::endl;
                cv::Mat frame;
                if (!cap.read(frame)) {
                    throw std::logic_error("Failed to get frame from cv::VideoCapture");
                }
                if (flipImage)
                    cv::flip(frame, frame, 1);

                // Infer results
                auto tInferenceBegins = cv::getTickCount();
                auto inferenceResults = faceDetector.detect(frame);
                for (auto& inferenceResult : inferenceResults) {
                    for (auto estimator : estimators) {
                        estimator->estimate(frame, inferenceResult);
                    }
                }
                auto tInferenceEnds = cv::getTickCount();

                // Measure FPS
                auto tIterationEnds = cv::getTickCount();
                overallTime = (tIterationEnds - tIterationBegins) * 1000. / cv::getTickFrequency();
                overallTimeAverager.updateValue(overallTime);
                tIterationBegins = tIterationEnds;

                inferenceTime = (tInferenceEnds - tInferenceBegins) * 1000. / cv::getTickFrequency();
                inferenceTimeAverager.updateValue(inferenceTime);

                if (FLAGS_pc) {
                    faceDetector.printPerformanceCounts();
                    for (auto const estimator : estimators) {
                        estimator->printPerformanceCounts();
                    }
                }

                if (FLAGS_r) {
                    for (auto& inferenceResult : inferenceResults) {
                        std::cout << inferenceResult << std::endl;
                    }
                }

                if (FLAGS_no_show) {
                    continue;
                }

                presenter.drawGraphs(frame);

                // Display the results
                for (auto const& inferenceResult : inferenceResults) {
                    resultsMarker.mark(frame, inferenceResult, res_data);
                }
                for(int i=0; i<51; i++)
                    std::cout<< i << ":" << res_data.keypoints[i] << std::endl;
                putTimingInfoOnFrame(frame, overallTimeAverager.getAveragedValue(),
                                    inferenceTimeAverager.getAveragedValue());
                cv::imshow(windowName, frame);

                // Controls the information being displayed while demo runs
                char key = static_cast<char>(cv::waitKey(delay));
                resultsMarker.toggle(key);

                // Press 'Esc' to quit, 'f' to flip the video horizontally
                if (key == 27)
                    break;
                else if (key == 'f')
                    flipImage = !flipImage;
                else
                    presenter.handleKey(key);
                
                std::cout << presenter.reportMeans() << '\n';
                send(new_socket , msg_down.c_str() , msg_down.size() , 0 );
                send(new_socket , &res_data.keypoints , sizeof(res_data.keypoints) , 0 ); }
            
            
            else if (!strncmp(rcv_msg, msg_close.c_str(), 4)) {
                std::cout << "CLOSE" << std::endl;
                send(new_socket , msg_down.c_str() , msg_down.size() , 0 );
                break; }
            
            
            else {
                std::cout << "UNKNOWN_MSG" << std::endl;
                break; }
        }
    }
    catch (const std::exception& error) {
        slog::err << error.what() << slog::endl;
        send(new_socket , msg_fail.c_str() , msg_fail.size() , 0 );
        return 1;
    }
    catch (...) {
        slog::err << "Unknown/internal exception happened." << slog::endl;
        send(new_socket , msg_fail.c_str() , msg_fail.size() , 0 );
        return 1;
    }
    slog::info << "Execution successful" << slog::endl;
    return 0;
}
