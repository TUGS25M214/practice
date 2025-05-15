#include <iostream>
#include <opencv2/opencv.hpp>

int main(){
    cv::Mat image0 = cv::imread("barbara.ppm", cv::IMREAD_COLOR_RGB);

    const int w = image0.cols; // 横
    const int h = image0.rows; // 縦
    const int nc = image0.channels();
    auto *p = image0.data; // address of top-left pixel

    for (int y = 0; y < h; ++y){
        for (int x = 0; x < w; ++x){
            for (int c = 0; c < nc; ++c){
                p[nc * (y * w + x) + c] /= 2;//課題RGBで読み込んだ値をBGRに並べ替える
            }
        }
    }

    cv::imshow("imshow window", image0);
    cv::waitKey(0);
    return 0;
}