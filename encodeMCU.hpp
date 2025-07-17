#include <vector>
#include <opencv2/opencv.hpp>
#include "ycctype.hpp"
#include "huffman_tables.hpp"
#include <x86intrin.h>
#include "zigzag_order.hpp"
#include "bitstream.hpp"

// clang-format off
/*
constexpr size_t zigzag_scan[64] = {
    0,   1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};
*/
// clang-format on

constexpr int BSIZ = 8;

void code(int, int){}

void encode_DC(int diff, int c, bitstream &cs){
    int abs_val = (diff < 0) ? -diff : diff;
    int bit_width = 0;
    
    int bound = 1;
    while (abs_val >= bound){
        bound += bound;
        bit_width++;
    }
    // _lzcunt_u32((unsigned int)abs_val)

    cs.put_bits(DC_cwd[c][bit_width], DC_len[c][bit_width]);
    if (bit_width != 0){
        if (diff < 0){
            diff -=1;
        }
        cs.put_bits(diff, bit_width);
    }
}

void encode_AC(int run, int val, int c, bitstream &cs){
    int abs_val = (val < 0) ? -val : val;
    int bit_width = 0;
    
    int bound = 1;
    while (abs_val >= bound){
        bound += bound;
        bit_width++;
    }
    // _lzcunt_u32((unsigned int)abs_val)

    cs.put_bits(AC_cwd[c][(run << 4) + bit_width], AC_len[c][(run << 4) + bit_width]);
    if (bit_width != 0){
        if (val < 0){
            val -=1;
        }
        cs.put_bits(val, bit_width);
    }
}

void encode_block(cv::Mat &input, int is_chrominance, int &previousDC, bitstream &cs){
    float *p = reinterpret_cast<float *>(input.data);
    int diff =static_cast<int>(p[0]) - previousDC;
    previousDC = static_cast<int>(p[0]);
    encode_DC(diff,is_chrominance, cs);

    // ZigZag scan (Look Up Table) + encode AC
    int zero_run = 0;
    for (size_t i = 1; i < 64; i++){
        int ac = p[zigzag_scan[i]];
        if (ac == 0){
            zero_run++;
        } else {
            while (zero_run > 15){
                //ZRL encoding()：ゼロレングスの符号化
                encode_AC(0xF, 0x0, is_chrominance, cs);
                zero_run -= 16;

            }
            //encodeAC()：非ゼロ係数の符号化
            encode_AC(zero_run, ac, is_chrominance, cs);
            zero_run = 0;
        }
    }
    if (zero_run){
        // EOB (End of Block)符号化
        encode_AC(0x0, 0x0, is_chrominance, cs);
    }
}

int encodeMCU(std::vector<cv::Mat> &ycrcb, ycctype ycc, bitstream &cs){
    cv::Mat blk;
    int previousDC[3] = {0};
    // MCUを構成
    for (int Ly = 0, Cy = 0; Ly < ycrcb[0].rows; Ly += BSIZ*ycc.YR, Cy += BSIZ){
        for (int Lx = 0, Cx = 0; Lx < ycrcb[0].cols; Lx += BSIZ * ycc.XR, Cx += BSIZ){
            // Y
            for (int y = 0; y < ycc.YR; ++y){
                for (int x = 0; x < ycc.XR; ++x){
                    blk = ycrcb[0](cv::Rect(Lx + BSIZ * x, Ly + BSIZ * y, BSIZ, BSIZ)).clone();
                    //encode Y block
                    encode_block(blk, 0, previousDC[0], cs);
                }
            }
            //Cb
            blk = ycrcb[2](cv::Rect(Cx, Cy, BSIZ, BSIZ)).clone();
            //encode Cb block
            encode_block(blk, 1, previousDC[2], cs);
            //Cr
            blk = ycrcb[1](cv::Rect(Cx, Cy, BSIZ, BSIZ)).clone();
            //encode Cr block
            encode_block(blk, 1, previousDC[1], cs);
        }
    }
    return EXIT_SUCCESS;
}