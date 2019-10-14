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

    timeLog.CheckBeginTest();

    int64_t frame_period_us = 500;
	int64_t deadline_us = 500;

    while(true)
    {
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

        // tagging - get pid, tid
        pid_t pid = getpid();
    	pid_t tid = gettid();

        // Tagging begin ///////////////////////////////////////////////////////////////////
        const char *job_1_name = "gpu_hog_calculator_1";
        tag_job_begin(pid, tid, job_1_name, frame_period_us, deadline_us,
        14L, false, true, 1UL);

        gpu_hog_calculator_1(image_roi);

        // Tag_end /////////////////////////////////////////////////////////////////////////
        tag_job_end(pid, tid, job_1_name);



        // Tagging begin ///////////////////////////////////////////////////////////////////
        const char *job_2_name = "gpu_hog_calculator_2";
        tag_job_begin(pid, tid, job_2_name, frame_period_us, deadline_us,
        14L, false, true, 1UL);

        gpu_hog_calculator_2(image_roi);

        // Tag_end /////////////////////////////////////////////////////////////////////////
        tag_job_end(pid, tid, job_2_name);


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

    std::cout << "Total : " << sum_time / 1000000 << " ms " << num_f << " frames \t" << std::endl;
    std::cout << "Ave : " <<  sum_time / num_f / 1000000 << " ms "  <<  1000 / (sum_time / num_f / 1000000) << " fps" << std::endl;
    std::cout << "Worst : " <<  max_time / 1000000 << " ms " <<  1000 / (max_time / 1000000) << " fps" << std::endl;


    return 0;
}
