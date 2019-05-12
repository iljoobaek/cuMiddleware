import os
import sys
import functools
from ctypes import cdll, c_uint, c_int, c_void_p, c_char_p, c_ulonglong, c_double
import ctypes

libc = cdll.LoadLibrary('libc.so.6')
libpytag = cdll.LoadLibrary("libpytag.so")

# Modify the res and argtypes of *MetaJob interface
libpytag.CreateMetaJob.restype = c_void_p
libpytag.CreateMetaJob.argtypes = [c_uint, c_int, c_char_p, 
                c_ulonglong, c_ulonglong, c_ulonglong,
                c_double, c_double, c_double,
                c_uint, c_ulonglong]
libpytag.DestroyMetaJob.restype = None
libpytag.DestroyMetaJob.argtypes = [c_void_p]

# Modify the res and argtypes of the TagState_* interface
libpytag.CreateTagStateObj.restype = c_void_p
libpytag.CreateTagStateObj.argtypes = [c_void_p]

libpytag.DestroyTagStateObj.restype = None
libpytag.DestroyTagStateObj.argtypes = [c_void_p] 

libpytag.TagState_acquire_gpu.restype = c_int
libpytag.TagState_acquire_gpu.argtypes = [c_void_p]

libpytag.TagState_release_gpu.restype = c_int
libpytag.TagState_release_gpu.argtypes = [c_void_p]

libpytag.TagState_start_timer.restype = None
libpytag.TagState_start_timer.argtypes = [c_void_p]

libpytag.TagState_end_timer.restype = None
libpytag.TagState_end_timer.argtypes = [c_void_p]

def gettid():
    SYS_gettid = 186 # SYS_gettid
    return libc.syscall(SYS_gettid)

class MetaJobStruct(object):
    def __init__(self, tid, priority, job_name,
            lpm=0, apm=0, wpm=0,
            let=0, aet=0, wet=0,
            run_count=0, deadline=0):
        # Ensure that string is in bytes before passing as c_char_p
        c_name = c_char_p(job_name.encode('utf-8'))

        self.mj = c_void_p(
                    libpytag.CreateMetaJob(tid, priority, c_name,
                    lpm, apm, wpm, let, aet, wet,
                    run_count, deadline)
                    )
        print("Alloc'd a MetaJob using CreateMetaJob:", self.mj)

    def __del__(self):
        libpytag.DestroyMetaJob(self.mj)
        print("Free'd a MetaJob using DestroyMetaJob:", self.mj)

class TagStateStruct(object):
    def __init__(self, init_meta_job_struct):
        self.ts = c_void_p(libpytag.CreateTagStateObj(init_meta_job_struct.mj))

    def __del__(self):
        libpytag.DestroyTagStateObj(self.ts)
       
    def acquire_gpu(self):
        return c_int(libpytag.TagState_acquire_gpu(self.ts)).value

    def release_gpu(self):
        return c_int(libpytag.TagState_release_gpu(self.ts)).value

    def start_timer(self):
        libpytag.TagState_start_timer(self.ts)

    def end_timer(self):
        libpytag.TagState_end_timer(self.ts)

def tag_fn(fn):
    def wrapper(*args, **kwargs):
        if wrapper.ts.acquire_gpu() < 0:
            print("Aborting job, couldn't acquire gpu!")
            raise Exception("Couldn't acquire gpu! Job too big!")
        res = wrapper.fn(*args, **kwargs)
        wrapper.ts.release_gpu()

        return res

    mjs = MetaJobStruct(gettid(), 0, fn.__name__, 0, 0, 0, 0, 0, 0, 0, 0)
    ts = TagStateStruct(mjs)
    wrapper.ts = ts
    wrapper.fn = fn
    return wrapper

if __name__ == "__main__":
    # Testing
    print("Testing TagState from python")
    name = "testing tag_state from python"
    p = MetaJobStruct(gettid(), 0, name, 0, 0, 0, 0, 0, 0, 0, 0)
    print("p is ", p.mj)
    print ("Creating tagstate obj")
    ts = TagStateStruct(p)
    print ("Acquiring gpu")
    res = ts.acquire_gpu()
    print ("Can run?", res)
    if res == 0:
        print("Sleeping 2 seconds")
        import time;
        time.sleep(2)
        print("woke up, continuing")

        ts.release_gpu()

