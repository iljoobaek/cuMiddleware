ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
GCC=gcc
CXX=g++
CUDAPATH?=/usr/local/cuda-10.0

all: libcuhook.so mid tag_lib.o

SHAREDFLAGS=-Wall -fPIC -shared -ldl -g -lrt
INCL_FLAGS=-I./include
MIDFLAGS=-Wall -g -Wl,--no-as-needed
EDIT_LD_PATH=LD_LIBRARY_PATH=$(ROOT_DIR)/lib
LOAD_MID=-Llib -lmid -lpthread -lrt
MID_LOAD=-lpthread -lrt

libcuhook.so: wrapcuda.cpp common.c mid_queue.c
	$(CXX) $(INCL_FLAGS) -I$(CUDAPATH)/include -o lib/libcuhook.so wrapcuda.cpp common.c mid_queue.c $(SHAREDFLAGS)

common.o: common.c
	$(GCC) $(INCL_FLAGS) $(MIDFLAGS) -o common.o common.c -c -fPIC

mid_queue.o: mid_queue.c
	$(GCC) $(INCL_FLAGS) $(MIDFLAGS) -o mid_queue.o mid_queue.c -c -fPIC

libmid.so: mid_queue.o common.o
	$(GCC) $(INCL_FLAGS) -shared -o lib/libmid.so mid_queue.o common.o

tag_lib.o: tag_lib.c
	$(GCC) $(INCL_FLAGS) $(MIDFLAGS) -o tag_lib.o tag_lib.c -c

tag_state.o: tag_state.cpp
	$(CXX) $(INCL_FLAGS) $(MIDFLAGS) -std=c++11 -o tag_state.o tag_state.cpp -c 

test_mid.o: test_mid.c libmid.so
	$(EDIT_LD_PATH) $(GCC) $(INCL_FLAGS) $(MIDFLAGS) -o test_mid.o test_mid.c $(LOAD_MID)

run_test_mid: test_mid.o
	$(EDIT_LD_PATH) ./test_mid.o;

test_tag.o: test_tag.c tag_lib.o libmid.so
	$(EDIT_LD_PATH) $(GCC) $(INCL_FLAGS) $(MIDFLAGS) -o test_tag.o test_tag.c tag_lib.o $(LOAD_MID)

run_test_tag: test_tag.o 
	$(EDIT_LD_PATH) ./test_tag.o;

test_tag_dec.o: test_tag_dec.cpp tag_state.o tag_lib.o libmid.so
	$(EDIT_LD_PATH) $(CXX) $(INCL_FLAGS) $(MIDFLAGS) -std=c++11 -o test_tag_dec.o test_tag_dec.cpp tag_state.o tag_lib.o $(LOAD_MID)

run_test_tag_dec: test_tag_dec.o
	$(EDIT_LD_PATH) ./test_tag_dec.o

mid: mymid.c mid_queue.o common.o
	$(EDIT_LD_PATH) $(GCC) $(INCL_FLAGS) $(MIDFLAGS) -o mid common.c mymid.c mid_queue.c $(MID_LOAD)

runmid: mid
	$(EDIT_LD_PATH) ./mid;

clean:
	rm -rf mid test_tag_dec test_tag
	rm -rf *.o
	rm -rf lib/*.so
