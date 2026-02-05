// OpenCV which is the videocapture and videowriter, also includes the greyscale
#include <opencv2/opencv.hpp>
// High resolution timing for performance measurement
#include <chrono>
// Math functions for sobel magnitude calculation
#include <cmath>
// Standard for 8-bit unsigned integer type
#include <cstdint>
// I/O for printing progress and final stats
#include <iostream>
// String for file path handling
#include <string>

// Timer from chrono that is used for performance measurement
using Clock = std::chrono::high_resolution_clock;

// Makes sure that a value fits the 0-255 range for a pixel
static inline uint8_t clampToU8(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
}

/*
  CPU Sobel baseline
  - Input: grayscale 8-bit image
  - Output: 8-bit binary edge mask
  - Method: Sobel 3x3 to a gradient magnitude to a threshold
        1) Compute Sobel gradient Gx and Gy using 3x3 kernels
        2) Compute magnitude = sqrt(Gx^2 + Gy^2)
        3) Clamp to 0-255
        4) Threshold -> 255 if >= threshold else black (0)
  - Border: Edges set to 0
*/
void sobelCpuBaseline(const cv::Mat& grayFrameU8, cv::Mat& edgeMaskU8, int edgeThreshold) {
    // Makes sure that input is single-channel 8-bit (CV_8UC1 is an OpenCV type code which means it is greyscale where each pixel is 1 byte)
    CV_Assert(grayFrameU8.type() == CV_8UC1);

    const int width  = grayFrameU8.cols;
    const int height = grayFrameU8.rows;

    // initialize everything to black (including borders)
    edgeMaskU8.create(grayFrameU8.size(), CV_8UC1);
    edgeMaskU8.setTo(0);

    // Loops only interior pixels to avoid out-of-bounds access
    for (int y = 1; y < height - 1; ++y) {
        const uint8_t* rowAbove = grayFrameU8.ptr<uint8_t>(y - 1);
        const uint8_t* rowHere  = grayFrameU8.ptr<uint8_t>(y);
        const uint8_t* rowBelow = grayFrameU8.ptr<uint8_t>(y + 1);
        uint8_t* outRow = edgeMaskU8.ptr<uint8_t>(y);

        for (int x = 1; x < width - 1; ++x) {
            // Sobel gradients in X and Y directions in the 3x3 layout
            const int gx =
                -rowAbove[x - 1] + rowAbove[x + 1] +
                -2 * rowHere[x - 1] + 2 * rowHere[x + 1] +
                -rowBelow[x - 1] + rowBelow[x + 1];

            const int gy =
                -rowAbove[x - 1] - 2 * rowAbove[x] - rowAbove[x + 1] +
                 rowBelow[x - 1] + 2 * rowBelow[x] + rowBelow[x + 1];

            // Edge strength magnitude
            const int magnitude =
                static_cast<int>(std::lround(std::sqrt(static_cast<double>(gx) * gx +
                                                     static_cast<double>(gy) * gy)));

            // Clamp for 8-bit output since sobel magnitude can exceed 255
            const uint8_t magnitudeU8 = clampToU8(magnitude);

            // Binary tresholding that makes strong edges white (255) and others black (0)
            outRow[x] = (magnitudeU8 >= edgeThreshold) ? 255 : 0;
        }
    }
}

// Returns milliseconds between two time points
static double msBetween(const Clock::time_point& start, const Clock::time_point& end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

// Main runs the CPU Sobel baseline on a video file, measures the per-stage timmings and writes output video if requested (GUI also optionally available)
int main(int argc, char** argv) {
    /*
      Command that is expected for usage, input_video is the path to a video file, output_video is the path to write the processed video (.avi format works with current setup), threshold
      is the edge threshold (default 80), display is whether to show a GUI window (0 = no, 1 = yes, default 0).
        sobel_cpu.exe <input_video> <output_video> [threshold=80] [display=0|1]
    */

    // CLI validation, checks for input and output video paths
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0]
                  << " <input_video> <output_video> [threshold=80] [display=0|1]\n";
        return 1;
    }

    // Read arguments (Threshold and display are optional)
    const std::string inputVideoPath  = argv[1];
    const std::string outputVideoPath = argv[2];
    const int edgeThreshold = (argc >= 4) ? std::stoi(argv[3]) : 80;
    const bool showWindow   = (argc >= 5) ? (std::stoi(argv[4]) != 0) : false;

    // Open video input, check for it not opening
    cv::VideoCapture videoReader(inputVideoPath);
    if (!videoReader.isOpened()) {
        std::cerr << "Failed to open input: " << inputVideoPath << "\n";
        return 1;
    }

    // Read input properties so that output can match the input dimensions and framerate
    const int frameWidth  = static_cast<int>(videoReader.get(cv::CAP_PROP_FRAME_WIDTH));
    const int frameHeight = static_cast<int>(videoReader.get(cv::CAP_PROP_FRAME_HEIGHT));
    const double inputFps = videoReader.get(cv::CAP_PROP_FPS);

    // Choose an output video codec (fourcc) to compress the frames to a video
    const int fourcc = cv::VideoWriter::fourcc('M','J','P','G');

    // Creates output writer only if an output path is given
    cv::VideoWriter videoWriter;
    if (!outputVideoPath.empty()) {
        // If input fps is not available, default to 30
        const double outputFps = (inputFps > 0.0) ? inputFps : 30.0;

        if (!videoWriter.open(outputVideoPath, fourcc, outputFps,
                      cv::Size(frameWidth, frameHeight), false)) {
    std::cerr << "Failed to open output: " << outputVideoPath << "\n";
    return 1;
    }
}

    // Frame buffers to avoid reallocating every frame, decoded input frame, greyscale frame, binary edges, BGR frame
    cv::Mat frameBgr;
    cv::Mat grayFrameU8;
    cv::Mat edgeMaskU8;
    cv::Mat edgeBgrForWriter;

    // Counters and timing totals for stats
    int processedFrames = 0;
    double sumDecodeMs = 0.0;
    double sumGrayMs   = 0.0;
    double sumSobelMs  = 0.0;
    double sumOutMs    = 0.0;

    // Starts the total fps calculation timer
    const auto totalStart = Clock::now();

    while (true) {
        // Read a frame
        const auto tDecodeStart = Clock::now();
        if (!videoReader.read(frameBgr)) break;
        const auto tDecodeEnd = Clock::now();

        // Convert to grayscale
        const auto tGrayStart = Clock::now();
        cv::cvtColor(frameBgr, grayFrameU8, cv::COLOR_BGR2GRAY);
        const auto tGrayEnd = Clock::now();

        // Sobel on CPU
        const auto tSobelStart = Clock::now();
        sobelCpuBaseline(grayFrameU8, edgeMaskU8, edgeThreshold);
        const auto tSobelEnd = Clock::now();

        // Output
        const auto outputStart = Clock::now();

        // Convert binary edge mask to BGR so video codecs that expect 3-channel input can accept it
        cv::cvtColor(edgeMaskU8, edgeBgrForWriter, cv::COLOR_GRAY2BGR);

        // Save the output frame if writer is enabled
        if (videoWriter.isOpened()) {
            videoWriter.write(edgeBgrForWriter);
        }
        
        // Show the output frame in a GUI window if requested
        if (showWindow) {
            cv::imshow("CPU Sobel Edges", edgeMaskU8);
            const int key = cv::waitKey(1);
            if (key == 27) break; // ESC
        }

        // End output timing
        const auto outputEnd = Clock::now();

        // Timing in milliseconds for each stage
        const double decodeMs = msBetween(tDecodeStart, tDecodeEnd);
        const double grayMs   = msBetween(tGrayStart, tGrayEnd);
        const double sobelMs  = msBetween(tSobelStart, tSobelEnd);
        const double outMs    = msBetween(outputStart, outputEnd);

        sumDecodeMs += decodeMs;
        sumGrayMs   += grayMs;
        sumSobelMs  += sobelMs;
        sumOutMs    += outMs;

        processedFrames++;
    }

    // Calculation for total runtime and average fps
    const auto totalEnd = Clock::now();
    const double totalSeconds = std::chrono::duration<double>(totalEnd - totalStart).count();
    const double avgFps = (totalSeconds > 0.0) ? (processedFrames / totalSeconds) : 0.0;

    // Prints for the stats
    if (processedFrames > 0) {
        std::cout << "\nCPU Baseline\n";
        std::cout << "Frames: " << processedFrames << "\n";
        std::cout << "Total time: " << totalSeconds << " s\n";
        std::cout << "Average FPS: " << avgFps << "\n";
        std::cout << "Average decode ms: " << (sumDecodeMs / processedFrames) << "\n";
        std::cout << "Average gray ms: " << (sumGrayMs   / processedFrames) << "\n";
        std::cout << "Average sobel ms: " << (sumSobelMs  / processedFrames) << "\n";
        std::cout << "Average out ms: " << (sumOutMs    / processedFrames) << "\n";
        std::cout << "Threshold: " << edgeThreshold << "\n";
    }

    return 0;
}
