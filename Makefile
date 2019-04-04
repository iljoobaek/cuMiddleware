CXX=g++
CUDAPATH?=/usr/local/cuda-10.0

all: libcuhook.so mid

SHAREDFLAGS=-Wall -fPIC -shared -ldl -g -lrt
MIDFLAGS=-Wall -g -Wl,--no-as-needed
MID_LOAD=-ldl -lrt -L./ -lpython3.5
TF_LOAD=-L./ -l:libtensorflow_framework.so -l:tf_kernels.so 

libcuhook.so: wrapcuda.cpp common.c wraputil.cpp mid_queue.c
	$(CXX) -I$(CUDAPATH)/include -o libcuhook.so wrapcuda.cpp common.c mid_queue.c wraputil.cpp $(SHAREDFLAGS)

mid: mymid.c mid_queue.c common.c 
	$(CXX) $(MIDFLAGS) -o mid common.c mymid.c mid_queue.c $(MID_LOAD) #$(TF_LOAD)
clean:
	rm libcuhook.so mid
