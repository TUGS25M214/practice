#include <iostream>
#include <opencv2/opencv.hpp>
#include <functional>
#include <string>
#include "encodeMCU.hpp"
#include "jpgheaders.hpp"
#include "ycctype.hpp"

struct params {
    int s;
    float scale;
};

// clang-format off
constexpr float qmatrix[2][64] = {
    // for Y
    {16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68, 109, 103, 77,
    24, 35, 55, 64, 81, 104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 99},
    // for Cb and Cr
    {17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99}};
    // clang-format on

void quantize(cv::Mat &input, params parameter){
    float *p = reinterpret_cast<float *>(input.data);
    for (int i = 0; i < BSIZ; i++){
        for (int j = 0; j < BSIZ; j++){
            p[i * BSIZ + j] /= qmatrix[parameter.s][i * BSIZ + j] * parameter.scale;
            p[i * BSIZ + j] = roundf(p[i * BSIZ + j]);
        }
    }
}

void dequantize(cv::Mat &input, params parameter){
    float *p = reinterpret_cast<float *>(input.data);
    for (int i = 0; i < BSIZ; i++){
        for (int j = 0; j < BSIZ; j++){
            p[i * BSIZ + j] *= qmatrix[parameter.s][i * BSIZ + j] * parameter.scale;
        }
    }
}

void fwddct2(cv::Mat &input, params s = {0, 0.0f}){
    cv::dct(input, input);
}

void inv_dct2(cv::Mat &input, params s = {0, 0.0f}){
    cv::idct(input, input);
}

void blkproc(cv::Mat &input, std::function<void(cv::Mat &, params)> func, params s = {0, 0.0f}){
    for (int y = 0; y < input.rows; y += BSIZ){
        for (int x = 0; x < input.cols; x += BSIZ){
            cv::Mat blk_in = input(cv::Rect(x, y, BSIZ, BSIZ)).clone();
            cv::Mat blk_out = input(cv::Rect(x, y, BSIZ, BSIZ));

            // blk_inに対して何らかの処理を加える（下の例は平均値で埋める処理）
            func(blk_in, s);

            blk_in.convertTo(blk_out, blk_out.type()); //元の画像のブロックに書き戻す
        }
    }
}


int main(int argc, char*argv[]){
    cv::Mat image0 = cv::imread("barbara.ppm", cv::IMREAD_COLOR_BGR);
    if (argc < 2){
        perror("1番目の引数が必要です");
        return EXIT_FAILURE;
    }
    const int w = image0.cols; // 横
    const int h = image0.rows; // 縦
    const int nc = image0.channels();

    const int XR = 2; // 水平方向の間引き率
    const int YR = 2; // 垂直方向の間引き率

    ycctype ycc(YCC::YUV420);


    // 圧縮画像の品質とそれに応じた量子化テーブルのスケールを計算
    int quality = std::stoi(argv[1]);
    if (quality < 0 || quality > 100){
        perror("1番目の引数はquality (0 - 100)です");
        return EXIT_FAILURE;
    }
    if (quality == 0){
        quality = 1;
    }
    float scale;
    if (quality < 50){
        scale = floorf(5000.0f / quality);
    } else {
        scale = 200.0f - 2.0f * quality;
    }
    scale /= 100.0f;
    if (quality == 100){
        scale = 1.0f;
    }

    params pLum, pCh;
    pLum.s = 0;
    pCh.s = 1;
    pLum.scale = pCh.scale = scale;

    int qtableL[64], qtableC[64];
    for (int i = 0; i < 64; ++i){
        float stepsize;

        stepsize = qmatrix[0][i] * scale;
        if (stepsize < 1.0f){
            stepsize = 1.0f;
        }
        if (stepsize > 255.0f){
            stepsize = 255.0f;
        }
        qtableL[i] = stepsize;

        stepsize = qmatrix[1][i] * scale;
        if (stepsize < 1.0f){
            stepsize = 1.0f;
        }
        if (stepsize > 255.0f){
            stepsize = 255.0f;
        }
        qtableC[i] = stepsize;
    }
    bitstream cs;
    create_mainheader(w, h, nc,qtableL, qtableC, ycc, cs);

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

    blkproc(ycrcb[0], fwddct2);
    blkproc(ycrcb[1], fwddct2);
    blkproc(ycrcb[2], fwddct2);
    blkproc(ycrcb[0], quantize, pLum); // Yの量子化テーブルを使用
    blkproc(ycrcb[1], quantize, pCh); // Cr, Cbの量子化テーブルを使用
    blkproc(ycrcb[2], quantize, pCh);
    encodeMCU(ycrcb, ycc, cs);
    auto codestream = cs.finalize();
    FILE *fp = fopen("output.jpg", "wb");
    fwrite(codestream.data(), sizeof(uint8_t), codestream.size(), fp);
    fclose(fp);
    //////////////////////////////////////////////////////////////////////////////////////
    blkproc(ycrcb[0], dequantize, pLum);
    blkproc(ycrcb[1], dequantize, pCh);
    blkproc(ycrcb[2], dequantize, pCh);
    blkproc(ycrcb[0], inv_dct2);
    blkproc(ycrcb[1], inv_dct2);
    blkproc(ycrcb[2], inv_dct2);
    

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