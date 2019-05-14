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
	$(GCC) -shared -o lib/libmid.so mid_queue.o common.o

tag_lib.o: tag_lib.c
	$(GCC) $(INCL_FLAGS) $(MIDFLAGS) -o tag_lib.o tag_lib.c -c -fPIC

tag_state.o: tag_state.cpp
	$(CXX) $(INCL_FLAGS) $(MIDFLAGS) -std=c++11 -o tag_state.o tag_state.cpp -c -fPIC

libpytag.so: tag_state.o tag_lib.o libmid.so
	$(EDIT_LD_PATH) $(CXX) -shared -o lib/libpytag.so tag_state.o tag_lib.o $(LOAD_MID)

test_mid.o: tests/test_mid.c libmid.so
	$(EDIT_LD_PATH) $(GCC) $(INCL_FLAGS) $(MIDFLAGS) -o tests/test_mid.o tests/test_mid.c $(LOAD_MID)

test_app: tests/app1.c tests/app2.c tag_lib.o libmid.so
	$(EDIT_LD_PATH) $(GCC) $(INCL_FLAGS) $(MIDFLAGS) -o tests/app1.o tests/app1.c tag_lib.o $(LOAD_MID)
	$(EDIT_LD_PATH) $(GCC) $(INCL_FLAGS) $(MIDFLAGS) -o tests/app2.o tests/app2.c tag_lib.o $(LOAD_MID)


run_test_mid: tests/test_mid.o
	$(EDIT_LD_PATH) ./tests/test_mid.o;

test_tag.o: tests/test_tag.c tag_lib.o libmid.so
	$(EDIT_LD_PATH) $(GCC) $(INCL_FLAGS) $(MIDFLAGS) -o tests/test_tag.o tests/test_tag.c tag_lib.o $(LOAD_MID)

run_test_tag: test_tag.o 
	$(EDIT_LD_PATH) ./tests/test_tag.o;

test_tag_dec.o: tests/test_tag_dec.cpp tag_state.o tag_lib.o libmid.so
	$(EDIT_LD_PATH) $(CXX) $(INCL_FLAGS) $(MIDFLAGS) -std=c++11 -o tests/test_tag_dec.o tests/test_tag_dec.cpp tag_state.o tag_lib.o $(LOAD_MID)

run_test_tag_dec: tests/test_tag_dec.o
	$(EDIT_LD_PATH) ./tests/test_tag_dec.o

mid: mymid.c mid_queue.o common.o
	$(EDIT_LD_PATH) $(GCC) $(INCL_FLAGS) $(MIDFLAGS) -o mid common.c mymid.c mid_queue.c $(MID_LOAD)

runmid: mid
	$(EDIT_LD_PATH) ./mid;

clean:
	rm -rf mid tests/test_tag_dec tests/test_tag
	rm -rf *.o
	rm -rf ./tests/*.o
	rm -rf lib/*.so
