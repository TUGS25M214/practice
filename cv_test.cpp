#include <iostream>
#include <opencv2/opencv.hpp>

int main(){
    cv::Mat image0 = cv::imread("barbara.ppm", cv::IMREAD_COLOR_BGR);

    const int w = image0.cols; // 横
    const int h = image0.rows; // 縦
    const int nc = image0.channels();
    const int XR = 32;
    const int YR = 32; 


    /*************** Encoder ***************/
    // 色空間返還: RGB -> YCbCr
    cv::cvtColor(image0, image0, cv::COLOR_BGR2YCrCb);
    auto *p = image0.data; // address of top-left pixel

    std::vector<cv::Mat> ycrcb;
    cv::split(image0, ycrcb);// Y, Cr ,Cbをそれぞれ独立したMatに分割

    // 色間引き処理
    cv::resize(ycrcb[1], ycrcb[1], cv::Size(), 1.0 / XR, 1.0 / YR, cv::INTER_LINEAR);
    cv::resize(ycrcb[2], ycrcb[2], cv::Size(), 1.0 / XR, 1.0 / YR, cv::INTER_LINEAR);


    /*************** Decoder ***************/
    //色差成分を補完
    cv::resize(ycrcb[1], ycrcb[1], cv::Size(), XR, YR, cv::INTER_LINEAR);
    cv::resize(ycrcb[2], ycrcb[2], cv::Size(), XR, YR, cv::INTER_LINEAR);
    cv::merge(ycrcb, image0);
    

    // 色空間返還: YCbCr -> RGB
    cv::cvtColor(image0, image0, cv::COLOR_YCrCb2BGR);

    cv::imshow("image0", image0);
    cv::waitKey(0);
    return 0;
}