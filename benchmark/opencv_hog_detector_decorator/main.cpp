/*
 * File:   main.cpp
 * Author: iljoo
 *
 * Created on April 16, 2019
*/

#include "opencv2/opencv.hpp"
#include "opencv2/video/tracking.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <opencv2/cudaobjdetect.hpp>
#include <stdio.h>
#include <iostream>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>

#include "TimeLog.hpp"
TimeLog timeLog;

// Tagging
#include <sys/types.h> // pid_t
#include "tag_frame.h" // FrameController
#include "tag_state.h" // gettid()
#include "tag_dec.h" // frame_job_decorator
#include "tag_gpu.h"

using namespace cv;
using namespace std;

// To enable GPU HOG
#define USE_GPU_HOG

#ifdef USE_GPU_HOG
cuda::Stream stream;
#endif

// to measure fps
struct timespec tpstart;
struct timespec tpend;
float diff_sec, diff_nsec;
int num_f;
float sum_time, max_time;

struct timespec tp_start_10;
struct timespec tp_now_10;
int frame_ctr;
float fps;
float total_nsec;

//int hog_Width = 400, hog_Height = 112;
//int hog_Width = 640, hog_Height = 360; // GTX 1070 - 4~5% utility
int hog_Width = 1280, hog_Height = 720; // GTX 1070 - 19~22% utility

int gpu_hog_calculator_1(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    for(int i=0; i<10; i++)
    {
        // copy image from cpu to gpu
        src_gpu.upload(image_roi);

        // color conversion
        cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

        // hog calculation
        gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

        // copy result from gpu to cpu
        Mat cpu_hog_descriptor;
        gpu_hog_descriptor.download(cpu_hog_descriptor);
        //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
        //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

        // release gpu memory
        src_gpu.release();
        mono_gpu.release();
        gpu_hog_descriptor.release();
    }
    return 1;
}

int gpu_hog_calculator_2(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    for(int i=0; i<10; i++)
    {
        // copy image from cpu to gpu
        src_gpu.upload(image_roi);

        // color conversion
        cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

        // hog calculation
        gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

        // copy result from gpu to cpu
        Mat cpu_hog_descriptor;
        gpu_hog_descriptor.download(cpu_hog_descriptor);
        //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
        //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

        // release gpu memory
        src_gpu.release();
        mono_gpu.release();
        gpu_hog_descriptor.release();
    }
    return 1;
}

int gpu_hog_calculator_layer_1(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}

int gpu_hog_calculator_layer_2(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}

int gpu_hog_calculator_layer_3(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}

int gpu_hog_calculator_layer_4(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}

int gpu_hog_calculator_layer_5(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}

int gpu_hog_calculator_layer_6(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}

int gpu_hog_calculator_layer_7(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}

int gpu_hog_calculator_layer_8(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}

int gpu_hog_calculator_layer_9(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}

int gpu_hog_calculator_layer_10(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}

int gpu_hog_calculator_layer_11(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}

int gpu_hog_calculator_layer_12(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}

int gpu_hog_calculator_layer_13(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}

int gpu_hog_calculator_layer_14(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}

int gpu_hog_calculator_layer_15(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}

int gpu_hog_calculator_layer_16(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}

int gpu_hog_calculator_layer_17(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}

int gpu_hog_calculator_layer_18(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}

int gpu_hog_calculator_layer_19(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}

int gpu_hog_calculator_layer_20(Mat image_roi)
{
    // GPU-related initialization
    cuda::GpuMat src_gpu, mono_gpu;
    cuda::GpuMat gpu_hog_descriptor;
    cv::Ptr<cv::cuda::HOG> gpu_hog = cv::cuda::HOG::create(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

    // copy image from cpu to gpu
    src_gpu.upload(image_roi);

    // color conversion
    cuda::cvtColor(src_gpu, mono_gpu, CV_BGR2GRAY);

    // hog calculation
    gpu_hog->compute(mono_gpu, gpu_hog_descriptor);

    // copy result from gpu to cpu
    Mat cpu_hog_descriptor;
    gpu_hog_descriptor.download(cpu_hog_descriptor);
    //cout << "GPU HoG size = " << gpu_hog->getDescriptorSize() << endl;
    //cout << "GPU HoG size = " << cpu_hog_descriptor.size() << endl;

    // release gpu memory
    src_gpu.release();
    mono_gpu.release();
    gpu_hog_descriptor.release();
    return 1;
}


int main(int argc, char** argv)
{
    Mat cur_img;

    // HoG value
    vector<float> descriptorsValues;

    cout<<"Started"<<endl;
    VideoCapture cap("../data/forbes_1920_1208_0_.mp4");

    // To meausure fps performance
    sum_time = 0;
    max_time = 0;
    clock_gettime(CLOCK_MONOTONIC, &tpstart);
    int frame_id = 0;

    /**********************************************************************/
    // To handle user input
    int desired_fps = 0;
    if (argc > 1) {
        desired_fps = atoi(argv[1]);
        //other parameters will be parsed in other function
        printf("Desired FPS %d\n", desired_fps);
    }
    else {
        printf("<usage>: ./xxxx <desired fps>\n");
        return -1;
    }
    /**********************************************************************/

    // tagging - decorator
    const char *frame_name = "opencv_hog_calculator";
	FrameController fc(frame_name, desired_fps, false);

    // decorate work functions
	auto tagged_work1_ptr = frame_job_decorator(gpu_hog_calculator_1, &fc, "gpu_hog_calculator_1", true);
	auto tagged_work2_ptr = frame_job_decorator(gpu_hog_calculator_2, &fc, "gpu_hog_calculator_2", true);
    /*auto tagged_work1_ptr = frame_job_decorator(gpu_hog_calculator_layer_1, &fc, "gpu_hog_calculator_layer_1", true);
    auto tagged_work2_ptr = frame_job_decorator(gpu_hog_calculator_layer_2, &fc, "gpu_hog_calculator_layer_2", true);
    auto tagged_work3_ptr = frame_job_decorator(gpu_hog_calculator_layer_3, &fc, "gpu_hog_calculator_layer_3", true);
    auto tagged_work4_ptr = frame_job_decorator(gpu_hog_calculator_layer_4, &fc, "gpu_hog_calculator_layer_4", true);
    auto tagged_work5_ptr = frame_job_decorator(gpu_hog_calculator_layer_5, &fc, "gpu_hog_calculator_layer_5", true);
    auto tagged_work6_ptr = frame_job_decorator(gpu_hog_calculator_layer_6, &fc, "gpu_hog_calculator_layer_6", true);
    auto tagged_work7_ptr = frame_job_decorator(gpu_hog_calculator_layer_7, &fc, "gpu_hog_calculator_layer_7", true);
    auto tagged_work8_ptr = frame_job_decorator(gpu_hog_calculator_layer_8, &fc, "gpu_hog_calculator_layer_8", true);
    auto tagged_work9_ptr = frame_job_decorator(gpu_hog_calculator_layer_9, &fc, "gpu_hog_calculator_layer_9", true);
    auto tagged_work10_ptr = frame_job_decorator(gpu_hog_calculator_layer_10, &fc, "gpu_hog_calculator_layer_10", true);
    auto tagged_work11_ptr = frame_job_decorator(gpu_hog_calculator_layer_11, &fc, "gpu_hog_calculator_layer_11", true);
    auto tagged_work12_ptr = frame_job_decorator(gpu_hog_calculator_layer_12, &fc, "gpu_hog_calculator_layer_12", true);
    auto tagged_work13_ptr = frame_job_decorator(gpu_hog_calculator_layer_13, &fc, "gpu_hog_calculator_layer_13", true);
    auto tagged_work14_ptr = frame_job_decorator(gpu_hog_calculator_layer_14, &fc, "gpu_hog_calculator_layer_14", true);
    auto tagged_work15_ptr = frame_job_decorator(gpu_hog_calculator_layer_15, &fc, "gpu_hog_calculator_layer_15", true);
    auto tagged_work16_ptr = frame_job_decorator(gpu_hog_calculator_layer_16, &fc, "gpu_hog_calculator_layer_16", true);
    auto tagged_work17_ptr = frame_job_decorator(gpu_hog_calculator_layer_17, &fc, "gpu_hog_calculator_layer_17", true);
    auto tagged_work18_ptr = frame_job_decorator(gpu_hog_calculator_layer_18, &fc, "gpu_hog_calculator_layer_18", true);
    auto tagged_work19_ptr = frame_job_decorator(gpu_hog_calculator_layer_19, &fc, "gpu_hog_calculator_layer_19", true);
    auto tagged_work20_ptr = frame_job_decorator(gpu_hog_calculator_layer_20, &fc, "gpu_hog_calculator_layer_20", true);*/

    timeLog.CheckBeginTest();

    while(true)
    {
        fc.frame_start();

        /* ***** START TIMING ***** */
        clock_gettime(CLOCK_MONOTONIC, &tpstart);
        //printf("%ld %ld\n", tpstart.tv_sec, tpstart.tv_nsec);
        /* ***** START TIMING ***** */

        frame_ctr++;
        //if(frame_ctr == 10)
        {
            clock_gettime(CLOCK_MONOTONIC, &tp_now_10);
            diff_sec = tp_now_10.tv_sec - tp_start_10.tv_sec;
            diff_nsec = tp_now_10.tv_nsec - tp_start_10.tv_nsec;
            total_nsec = diff_sec * 1000000000 + diff_nsec;
            fps = 1 / (total_nsec / 1000000000);
            std::cout << "FPS: " << fps << std::endl;
            frame_ctr = 0;
            clock_gettime(CLOCK_MONOTONIC, &tp_start_10);
        }
        frame_id++;


        // Get Image
        if(!(cap.read(cur_img)))
            break;
        if(cur_img.empty())
            break;


        Rect roi = Rect(0, 0, hog_Width, hog_Height); // x, y, width, height
        Mat image_roi = cur_img(roi);

        /**********************************************************************/
        #ifdef USE_GPU_HOG

        //gpu_hog_calculator_1(image_roi);
        //gpu_hog_calculator_2(image_roi);

        (*tagged_work1_ptr)(image_roi);
		(*tagged_work2_ptr)(image_roi);

		/*(*tagged_work1_ptr)(image_roi);
		(*tagged_work2_ptr)(image_roi);
        (*tagged_work3_ptr)(image_roi);
		(*tagged_work4_ptr)(image_roi);
        (*tagged_work5_ptr)(image_roi);
		(*tagged_work6_ptr)(image_roi);
        (*tagged_work7_ptr)(image_roi);
		(*tagged_work8_ptr)(image_roi);
        (*tagged_work9_ptr)(image_roi);
		(*tagged_work10_ptr)(image_roi);
        (*tagged_work11_ptr)(image_roi);
		(*tagged_work12_ptr)(image_roi);
        (*tagged_work13_ptr)(image_roi);
		(*tagged_work14_ptr)(image_roi);
        (*tagged_work15_ptr)(image_roi);
		(*tagged_work16_ptr)(image_roi);
        (*tagged_work17_ptr)(image_roi);
		(*tagged_work18_ptr)(image_roi);
        (*tagged_work19_ptr)(image_roi);
		(*tagged_work20_ptr)(image_roi);*/

        #else /*USE_GPU_HOG*/
        Mat gray;
        vector<float> descriptorsValues;
        HOGDescriptor hog(Size(hog_Width, hog_Height), Size(16,16), Size(8,8), Size(8,8), 9);

        // gray conversion
        cvtColor(image_roi, gray, CV_BGR2GRAY);

        // hog calculation : hog size
        hog.compute(gray, descriptorsValues, Size(0,0), Size(0,0));
        //cout << "HoG size = " << descriptorsValues.size() << endl;
        #endif /*USE_GPU_HOG*/
        /**********************************************************************/

        /* ***** END TIMING ***** */
        clock_gettime(CLOCK_MONOTONIC, &tpend);
        //printf("%ld %ld\n", tpend.tv_sec, tpend.tv_nsec);
        /*std::sort(flow.begin(), flow.end());
        if (flow.size() > 0)
            medianFlow = flow[flow.size()/2];
        */
        diff_sec = tpend.tv_sec - tpstart.tv_sec;
        diff_nsec = tpend.tv_nsec - tpstart.tv_nsec;
        total_nsec = diff_sec * 1000000000 + diff_nsec;
        if(frame_id > 15)
        {
            if(total_nsec > max_time)
                max_time = total_nsec;
            sum_time += total_nsec;
            num_f++;
        }
        /* ***** END TIMING ***** */

        fc.frame_end();

        char cKey = waitKey(5);
        if ( cKey == 27) // ESC
            break;
        else if (cKey == 32) // Space
        {
            while(1)
            {
                if(waitKey(33) >= 0)
                    break;
            }
        }

        if (FILE *file = fopen("../stop_benchmark.txt", "r")) {
            fclose(file);
            break;
        }
    }
    fc.print_exec_stats();

    std::cout << "Total : " << sum_time / 1000000 << " ms " << num_f << " frames \t" << std::endl;
    std::cout << "Ave : " <<  sum_time / num_f / 1000000 << " ms "  <<  1000 / (sum_time / num_f / 1000000) << " fps" << std::endl;
    std::cout << "Worst : " <<  max_time / 1000000 << " ms " <<  1000 / (max_time / 1000000) << " fps" << std::endl;


    return 0;
}
