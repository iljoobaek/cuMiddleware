import os
import sys
import functools
from ctypes import cdll, c_uint, c_int, c_void_p, c_char_p, c_ulonglong, c_double

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

    def __del__(self):
        libpytag.DestroyMetaJob(self.mj)

class TagStateStruct(object):
    def __init__(self, init_meta_job_struct):
        self.ts = c_void_p(libpytag.CreateTagStateObj(init_meta_job_struct.mj))
        # Must hold a pointer to mj so the memory isn't free'd!
        self.mj = init_meta_job_struct

    def __del__(self):
        libpytag.DestroyTagStateObj(self.ts)
        # Removes reference to self.mj, so it can garbage collected now

    def acquire_gpu(self):
        return c_int(libpytag.TagState_acquire_gpu(self.ts)).value

    def release_gpu(self):
        return c_int(libpytag.TagState_release_gpu(self.ts)).value

    def start_timer(self):
        libpytag.TagState_start_timer(self.ts)

    def end_timer(self):
        libpytag.TagState_end_timer(self.ts)

def tag_fn(fn, name):
    @functools.wraps(fn)
    def wrapper(*args, **kwargs):
        if wrapper.ts.acquire_gpu() < 0:
            print("Aborting job, couldn't acquire gpu!")
            raise Exception("Couldn't acquire gpu! Job too big!")
        res = wrapper.fn(*args, **kwargs)
        wrapper.ts.release_gpu()

        return res

    mjs = MetaJobStruct(gettid(), 0, name, 0, 0, 0, 0, 0, 0, 0, 0)
    ts = TagStateStruct(mjs) # ts will hold a ref to mjs
    wrapper.ts = ts
    wrapper.fn = fn
    print("Installed tagging routine for fn (name: %s): %s" % (name, fn.__name__))
    return wrapper

def tag_tf_layer(layer_obj):
    raise NotImplementedError()
    import tensorflow as tf
    import types
    # Manually decorate the layer_obj's __call__ function with tagging routine
    assert(isinstance(layer_obj, tf.keras.layers.Layer))
    print("Wrapping layer (%s) obj's __call__ fn with tagging!" % layer_obj.name)
    wrapped_call = tag_fn(layer_obj.__call__, layer_obj.__name__ + "__call__")

    layer_obj.__call__ = types.MethodType(wrapped_call, layer_obj)
    return layer_obj

def tag_all_tf_layers(enable=True):
    # Dynamically tag all tf layers by overriding Keras/TF-slim's base
    # Layer object's __init__ function. Then, manually decorate the
    # object's __call__ (aliased by obj's 'apply()' method using tag_fn wrapper
    raise NotImplementedError()
    if enable:
        import tensorflow as tf
        TFBaseLayer = tf.keras.layers.Layer
        orig_init = TFBaseLayer.__init__
        def override_init(self, *args, **kwargs):
            orig_init(self, *args, **kwargs)
            print("Manually decorating layer (%s) __call__ with tag library!" % self.name)
            self = tag_tf_layer(self)

        TFBaseLayer.__init__ = override_init

        return True
    else:
        return False

def tag_pytorch_nn_layer(pt_module_obj):
    """
    Monkey-patching function called within __init__ 
        to every object of (inherited) type torch.nn.Module
    Installs tagging routine to pt_module_obj around 'forward()' fn 
    only if obj is a 'leaf' Module class, i.e. it has no children Modules.
    Returns None
    Throws assertion exception if obj is not of type torch.nn.Module
    """
    import torch as pt
    import types
    # Manually decorate the layer_obj's 'forward()' function with tagging routine
    assert(isinstance(pt_module_obj, pt.nn.Module))

    # Only tag 'leaf' Modules 
    n_children = sum(1 for _ in pt_module_obj.children())
    if n_children > 0:
        # Function is no-op for non-leaf Modules
        return
    elif hasattr(pt_module_obj, "__tagged"):
        # Already wrapped
        print("Already wrapped Pytorch module (%s) obj's forward fn with tagging!" % pt_module_obj.__class__)
        return
    else:
        print("Wrapping Pytorch module (%s) obj's forward fn with tagging!" % pt_module_obj.__class__)
        wrapped_call = tag_fn(pt_module_obj.forward, str(pt_module_obj.__class__) +".forward")
        #pt_module_obj.forward = types.MethodType(wrapped_call, pt_module_obj)
        pt_module_obj.forward = wrapped_call

        # Mark this object as tagged
        pt_module_obj.__tagged = True
        return

def tag_all_pt_submodules(cls, enable=True):
    # Dynamically tag all Pytorch layers by overriding nn's base
    # Module subclass' __init__ function. Then, manually decorate the
    # object's forward (called within __call__) method using tag_fn wrapper
    if enable:
        orig_init = cls.__init__
        def override_init(self, *args, **kwargs):
            res = orig_init(self, *args, **kwargs)
            # Monkey-patch forward after original __init__
            self.apply(tag_pytorch_nn_layer)
            return res

        cls.__init__ = override_init 

        return True
    else:
        return False

def tag_pytorch_nn_functions_list(fn_names, enable=True):
    # Dynamically tag all designated Pytorch functional operations 
    # So that all forward passes through layers tag these designated laye
    # operations
    # i.e. ["conv2d", "batch_norm", "avg_pool2d", "linear", "relu"]
    if enable:
        import importlib
        torch_fn_module = importlib.import_module("torch.nn.functional")
        for fn_name in fn_names:
            orig_fn = getattr(torch_fn_module, fn_name)
            tagged_fn = tag_fn(orig_fn, fn_name)
            setattr(torch_fn_module, fn_name, tagged_fn) 
        print("Tagged torch.nn.functional." + str(fn_names) + " successfully")
        return True
    else:
        return False


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

