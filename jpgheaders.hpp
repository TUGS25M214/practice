#pragma once

#include "bitstream.hpp"
#include "ycctype.hpp"
void create_mainheader(int width, int height, int nc, int *qtable_L, int *qtable_C, ycctype &ycc,
                       bitstream &enc);