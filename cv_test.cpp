#include <iostream>
#include <opencv2/opencv.hpp>
#include <functional>

constexpr int BSIZ = 8;

void quantize(cv::Mat &input){
    float *p = reinterpret_cast<float *>(input.data);
    for (int i = 0; i < BSIZ; i++){
        for (int j = 0; j < BSIZ; j++){
            p[i * BSIZ + j] /= 16;
            p[i * BSIZ + j] /= roundf(p[i * BSIZ + j]);
        }
    }
}

void dequantize(cv::Mat &input){
    float *p = reinterpret_cast<float *>(input.data);
    for (int i = 0; i < BSIZ; i++){
        for (int j = 0; j < BSIZ; j++){
            p[i * BSIZ + j] *= 32;
        }
    }
}

void fwddct2(cv::Mat &input){
    cv::dct(input, input);
}

void inv_dct2(cv::Mat &input){
    cv::idct(input, input);
}

void blkproc(cv::Mat &input, std::function<void(cv::Mat &)> func){
    for (int y = 0; y < input.rows; y += BSIZ){
        for (int x = 0; x < input.cols; x += BSIZ){
            cv::Mat blk_in = input(cv::Rect(x, y, BSIZ, BSIZ)).clone();
            cv::Mat blk_out = input(cv::Rect(x, y, BSIZ, BSIZ));

            // blk_inに対して何らかの処理を加える（下の例は平均値で埋める処理）
            func(blk_in);

            blk_in.convertTo(blk_out, blk_out.type()); //元の画像のブロックに書き戻す
        }
    }
}

int main(){
    cv::Mat image0 = cv::imread("barbara.ppm", cv::IMREAD_COLOR_BGR);

    const int w = image0.cols; // 横
    const int h = image0.rows; // 縦
    const int nc = image0.channels();

    const int XR = 2; // 水平方向の間引き率
    const int YR = 2; // 垂直方向の間引き率


    /*************** Encoder ***************/
    // 色空間返還: RGB -> YCbCr
    cv::cvtColor(image0, image0, cv::COLOR_BGR2YCrCb);
    auto *p = image0.data; // address of top-left pixel

    std::vector<cv::Mat> ycrcb;
    cv::split(image0, ycrcb); // Y, Cr ,Cbをそれぞれ独立したMatに分割

    // 色間引き処理
    cv::resize(ycrcb[1], ycrcb[1], cv::Size(), 1.0 / XR, 1.0 / YR, cv::INTER_LINEAR);
    cv::resize(ycrcb[2], ycrcb[2], cv::Size(), 1.0 / XR, 1.0 / YR, cv::INTER_LINEAR);

    // float(32bit)に変換
    ycrcb[0].convertTo(ycrcb[0], CV_32F);
    ycrcb[1].convertTo(ycrcb[1], CV_32F);
    ycrcb[2].convertTo(ycrcb[2], CV_32F);

    // DCレベルシフト  
    ycrcb[0] -= 128.0f;
    ycrcb[1] -= 128.0f;
    ycrcb[2] -= 128.0f;

    blkproc(ycrcb[0],fwddct2);
    blkproc(ycrcb[1],fwddct2);
    blkproc(ycrcb[2],fwddct2);
    blkproc(ycrcb[0],quantize);
    blkproc(ycrcb[1],quantize);
    blkproc(ycrcb[2],quantize);
    
    //////////////////////////////////////////////////////////////////////////////////////
    blkproc(ycrcb[0],dequantize);
    blkproc(ycrcb[1],dequantize);
    blkproc(ycrcb[2],dequantize);
    blkproc(ycrcb[0],inv_dct2);
    blkproc(ycrcb[1],inv_dct2);
    blkproc(ycrcb[2],inv_dct2);
    

    // DCレベルシフト  
    ycrcb[0] += 128.0f;
    ycrcb[1] += 128.0f;
    ycrcb[2] += 128.0f;

    // uint8_t(8bit)に変換
    ycrcb[0].convertTo(ycrcb[0], CV_8U);
    ycrcb[1].convertTo(ycrcb[1], CV_8U);
    ycrcb[2].convertTo(ycrcb[2], CV_8U);


    /*************** Decoder ***************/
    // 色差成分を補完
    cv::resize(ycrcb[1], ycrcb[1], cv::Size(), XR, YR, cv::INTER_LINEAR);
    cv::resize(ycrcb[2], ycrcb[2], cv::Size(), XR, YR, cv::INTER_LINEAR);
    cv::merge(ycrcb, image0);


    // 色空間返還: YCbCr -> RGB
    cv::cvtColor(image0, image0, cv::COLOR_YCrCb2BGR);

    cv::imshow("image0", image0);
    cv::waitKey(0);
    return 0;
}