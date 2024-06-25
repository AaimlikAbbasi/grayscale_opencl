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

int main() {
    printf("Starting the program.\n");

    // Load colored image using stb_image
    printf("Loading the image.\n");
    int width, height, channels;
    unsigned char *image = stbi_load("ISIC_0178428.jpg", &width, &height, &channels, 0);
    if (!image) {
        printf("Failed to load image.\n");
        return 1;
    }

    // Set up OpenCL
    printf("Setting up OpenCL.\n");
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;

    // Get platform and device
    printf("Getting platform and device information.\n");
    cl_int ret = clGetPlatformIDs(1, &platform, NULL);
    if (ret != CL_SUCCESS) {
        printf("Failed to get platform IDs.\n");
        return 1;
    }
    ret = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, NULL);
    if (ret != CL_SUCCESS) {
        printf("Failed to get device IDs.\n");
        return 1;
    }

    // Create context
    printf("Creating OpenCL context.\n");
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &ret);
    if (ret != CL_SUCCESS) {
        printf("Failed to create context.\n");
        return 1;
    }

    // Create command queue
    printf("Creating command queue.\n");
    queue = clCreateCommandQueueWithProperties(context, device, NULL, &ret);
    if (ret != CL_SUCCESS) {
        printf("Failed to create command queue.\n");
        return 1;
    }

    // Load kernel source
    printf("Loading kernel source.\n");
    program = clCreateProgramWithSource(context, 1, (const char **)&kernelSource, NULL, &ret);
    if (ret != CL_SUCCESS) {
        printf("Failed to create program with source.\n");
        return 1;
    }

    // Build the program
    printf("Building the program.\n");
    ret = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (ret != CL_SUCCESS) {
        printf("Error building program: %d\n", ret);
        char buildLog[4096];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(buildLog), buildLog, NULL);
        printf("Build log:\n%s\n", buildLog);
        return 1;
    }

    // Create kernel
    printf("Creating kernel.\n");
    kernel = clCreateKernel(program, "rgb_to_gray", &ret);
    if (ret != CL_SUCCESS) {
        printf("Failed to create kernel.\n");
        return 1;
    }

    // Create memory buffers
    printf("Creating memory buffers.\n");
    cl_mem inputBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(unsigned char) * width * height * channels, image, &ret);
    if (ret != CL_SUCCESS) {
        printf("Failed to create input buffer.\n");
        return 1;
    }
    cl_mem outputBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(unsigned char) * width * height, NULL, &ret);
    if (ret != CL_SUCCESS) {
        printf("Failed to create output buffer.\n");
        return 1;
    }

    // Set kernel arguments
    printf("Setting kernel arguments.\n");
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuffer);
    ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputBuffer);
    ret |= clSetKernelArg(kernel, 2, sizeof(int), &width);
    ret |= clSetKernelArg(kernel, 3, sizeof(int), &height);
    if (ret != CL_SUCCESS) {
        printf("Failed to set kernel arguments.\n");
        return 1;
    }

    // Execute kernel
    printf("Executing kernel.\n");
    size_t globalWorkSize[2] = { width, height };
    ret = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
    if (ret != CL_SUCCESS) {
        printf("Failed to enqueue kernel.\n");
        return 1;
    }

    // Read output buffer
    printf("Reading output buffer.\n");
    unsigned char *outputImage = (unsigned char *)malloc(sizeof(unsigned char) * width * height);
    ret = clEnqueueReadBuffer(queue, outputBuffer, CL_TRUE, 0, sizeof(unsigned char) * width * height, outputImage, 0, NULL, NULL);
    if (ret != CL_SUCCESS) {
        printf("Failed to read output buffer.\n");
        return 1;
    }

    // Create directory to store output images
    printf("Creating directory to store output images.\n");
    if (system("mkdir -p /output") != 0) {
        printf("Failed to create output directory.\n");
        return 1;
    }

    // Save output image in the output folder
    printf("Saving output image.\n");
    char outputFileName[100];
    snprintf(outputFileName, sizeof(outputFileName), "/output.jpg");
    int success = stbi_write_jpg(outputFileName, width, height, 1, outputImage, 100);
    if (!success) {
        printf("Failed to save output image.\n");
        return 1;
    }

    // Cleanup
    printf("Cleaning up.\n");
    ret = clReleaseMemObject(inputBuffer);
    ret |= clReleaseMemObject(outputBuffer);
    ret |= clReleaseKernel(kernel);
    ret |= clReleaseProgram(program);
    ret |= clReleaseCommandQueue(queue);
    ret |= clReleaseContext(context);
    if (ret != CL_SUCCESS) {
        printf("Failed to clean up resources.\n");
        return 1;
    }

  
    printf("Freeing memory.\n");
    stbi_image_free(image);
    free(outputImage);

    printf("Program finished successfully.\n");
    return 0;
}

