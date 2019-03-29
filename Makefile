CXX=g++
CUDAPATH?=/usr/local/cuda-10.0

all: libcuhook.so

COMMONFLAGS=-Wall -fPIC -shared -ldl -g

libcuhook.so: wrapcuda.cpp wrapcublas.cpp
	$(CXX) -I$(CUDAPATH)/include $(COMMONFLAGS) -o libcuhook.so wrapcuda.cpp wrapcublas.cpp

clean:
	-rm *.*o
