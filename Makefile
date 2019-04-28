GCC=gcc
CXX=g++
CUDAPATH?=/usr/local/cuda-10.0

all: libcuhook.so mid

SHAREDFLAGS=-Wall -fPIC -shared -ldl -g -lrt
MIDFLAGS=-Wall -g -Wl,--no-as-needed
MID_LOAD=-lpthread -lrt #-ldl -lrt -L./ -lpython3.5
TF_LOAD=-L./ -l:libtensorflow_framework.so -l:tf_kernels.so

libcuhook.so: wrapcuda.cpp common.c wraputil.cpp mid_queue.c
	$(CXX) -I$(CUDAPATH)/include -o libcuhook.so wrapcuda.cpp common.c mid_queue.c wraputil.cpp $(SHAREDFLAGS)

common.o: common.c
	$(GCC) $(MIDFLAGS) -o common.o common.c $(MID_LOAD) -c

mid_queue.o: mid_queue.c
	$(GCC) $(MIDFLAGS) -o mid_queue.o mid_queue.c -c

tag_lib.o: tag_lib.c mid_queue.o common.o
	$(GCC) $(MIDFLAGS) -o tag_lib.o tag_lib.c mid_queue.o common.o $(MID_LOAD) -c

test_mid.o: test_mid.c mid_queue.o common.o
	$(GCC) $(MIDFLAGS) -o test_mid.o test_mid.c mid_queue.o common.o $(MID_LOAD)

test_tag.o: test_tag.c tag_lib.o common.o mid_queue.o
	$(GCC) $(MIDFLAGS) -o test_tag.o test_tag.c tag_lib.o common.o mid_queue.o $(MID_LOAD)

mid: mymid.c mid_queue.o common.o
	$(GCC) $(MIDFLAGS) -o mid common.c mymid.c mid_queue.c $(MID_LOAD) #$(TF_LOAD)

test_server: mid test_tag.o

clean:
	rm -rf libcuhook.so mid
	rm -rf *.o
