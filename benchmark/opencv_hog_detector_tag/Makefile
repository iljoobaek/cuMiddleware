CXX=g++
CFLAGS=-O3

# OpenCV
INCLUDES_OPENCV = /usr/local/include/opencv2
LIB_PATH_OPENCV = /usr/local/lib
OPENCV_LDFLAGS= -L$(LIB_PATH_OPENCV) -lopencv_core -lopencv_flann -lopencv_imgproc -lopencv_imgcodecs -lopencv_videoio -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_objdetect -lopencv_features2d -lopencv_calib3d -lopencv_superres -lopencv_cudaarithm -lopencv_cudafilters -lopencv_cudawarping -lopencv_cudaimgproc -lopencv_cudafeatures2d -lopencv_cudaoptflow -lopencv_cudabgsegm -lopencv_cudastereo -lopencv_cudaobjdetect

# Tagging
INCLUDES_TAG = /home/iljoo/cuMiddleware_work/cuMiddleware_iljoo/include
LIB_PATH_TAG = /home/iljoo/cuMiddleware_work/cuMiddleware_iljoo/lib
TAG_LDFLAGS = -L$(LIB_PATH_TAG) -lmid -lpytag -lpthread -lrt

# Main application with Tagging
hogdetector : main.cpp
	$(CXX) $(CFLAGS) main.cpp -o hog -I$(INCLUDES_OPENCV) $(OPENCV_LDFLAGS) -I$(INCLUDES_TAG) $(TAG_LDFLAGS)

clean:
	rm -f *~ hog *.o
