CC = g++ -std=c++11

INCLUDES_OPENCV = /home/iljoo/opencv_3.3.1/include/opencv
LIB_PATH_OPENCV = /home/iljoo/opencv_3.3.1/lib

CFLAGS = -Wall -I. -I/usr/local/cuda-10.0/include -I/usr/include/x86-64-linux-gnu/ -I$(INCLUDES_OPENCV)
LDFLAGS = -L$(LIB_PATH_OPENCV)  -L/usr/lib/x86_64-linux-gnu/ -L/usr/local/cuda-10.0/lib64/stubs
DEPS = shader.hpp texture.hpp objloader.hpp
OBJ = surround_view.o shader.o texture.o objloader.o ../common/TimeLog.o GpuLog.o
TARGET = surround_view

LIBS=-lGL -lGLEW -lGLU -lGLEW -lglfw -lglut -lopencv_shape -lopencv_stitching -lopencv_objdetect -lopencv_superres -lopencv_videostab -lopencv_calib3d -lopencv_features2d -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_videoio -lopencv_photo -lopencv_ml -lopencv_imgproc -lopencv_flann -lopencv_core -lavcodec -lavformat -lswscale -lavutil -lX11 -lXrandr -lXi -lXxf86vm -lpthread -ldl -lXinerama -lXcursor -lrt -lm
#-lnvidia-ml -lGLESv2

# Tagging
CPP_FLAGS=-std=c++11
MIDFLAGS=-Wall -g -Wl,--no-as-needed
INCLUDES_TAG = /home/iljoo/cuMiddleware_work/cuMiddleware/include
LIB_PATH_TAG = /home/iljoo/cuMiddleware_work/cuMiddleware/lib
TAG_LDFLAGS = -L$(LIB_PATH_TAG) -lmid -lcarss -lpthread -lrt

.PHONY = clean

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -I$(INCLUDES_TAG) $(TAG_LDFLAGS)

$(TARGET): $(OBJ)
	g++ -o $@ $^ $(CFLAGS) $(CPP_FLAGS) $(MIDFLAGS) $(LDFLAGS) $(LIBS) -I$(INCLUDES_TAG) $(TAG_LDFLAGS)
	-rm *.o

clean:
	-rm $(TARGET)
