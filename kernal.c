
#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
const char *kernelSource = "\
    __kernel void rgb_to_gray(__global unsigned char* inputImage, __global unsigned char* outputImage, const int width, const int height) {\
        int x = get_global_id(0);\
        int y = get_global_id(1);\
        int index = y * width + x;\
        outputImage[index] = (unsigned char)(0.299f * inputImage[3 * index] + 0.587f * inputImage[3 * index + 1] + 0.114f * inputImage[3 * index + 2]);\
    }";
