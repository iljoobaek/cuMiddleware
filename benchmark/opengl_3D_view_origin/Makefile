CC = g++ -std=c++11

INCLUDES_OPENCV = /home/iljoo/opencv_3.3.1/include/opencv
LIB_PATH_OPENCV = /home/iljoo/opencv_3.3.1/lib

CFLAGS = -Wall -I. -I../common -I/usr/local/cuda-10.0/include -I/usr/include/x86-64-linux-gnu/ -I$(INCLUDES_OPENCV)
LDFLAGS = -L$(LIB_PATH_OPENCV)  -L/usr/lib/x86_64-linux-gnu/ -L/usr/local/cuda-10.0/lib64/stubs
DEPS = shader.hpp texture.hpp objloader.hpp
OBJ = surround_view.o shader.o texture.o objloader.o GpuLog.o ../common/TimeLog.o
TARGET = surround_view

LIBS=-lGL -lGLEW -lGLU -lGLEW -lglfw -lglut -lopencv_shape -lopencv_stitching -lopencv_objdetect -lopencv_superres -lopencv_videostab -lopencv_calib3d -lopencv_features2d -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_videoio -lopencv_photo -lopencv_ml -lopencv_imgproc -lopencv_flann -lopencv_core -lavcodec -lavformat -lswscale -lavutil -lX11 -lXrandr -lXi -lXxf86vm -lpthread -ldl -lXinerama -lXcursor -lrt -lm
#-lnvidia-ml -lGLESv2

.PHONY = clean

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	g++ -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)
	-rm *.o

clean:
	-rm $(TARGET)
