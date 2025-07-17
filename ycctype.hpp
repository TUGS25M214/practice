#pragma once // インクルードガード：ヘッダーファイルの読み込みを1回に制限

enum YCC {
    YUV444,
    YUV422,
    YUV420,
    YUV411,
    YUV440,
    YUV410,
    GRAY
};

constexpr int subsample_factor[7][2]= {
    // {H, V} 輝度成分のMCU内の8*8ブロックの{水平、垂直}数
    {1, 1}, // 444
    {2, 1}, // 422
    {2, 2}, // 420
    {1, 2}, // 440
    {4, 2}, // 410
    {1, 1} // GRAY
};

struct ycctype {
    const int XR;
    const int YR;
    ycctype(int n) : XR(subsample_factor[n][0]), YR(subsample_factor[n][1]) {}
};