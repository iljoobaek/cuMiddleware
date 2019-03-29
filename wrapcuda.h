#ifndef _WRAPCUDA_H_
# define _WRAPCUDA_H_
#include <cuda.h>
#include <driver_types.h>

// In case you want to intercept, the callbacks need the same type/parameters as the real functions
typedef CUresult CUDAAPI (*fnMemAlloc)(CUdeviceptr *dptr, size_t bytesize);
typedef CUresult CUDAAPI (*fnMemFree)(CUdeviceptr dptr);
typedef CUresult CUDAAPI (*fnCtxGetCurrent)(CUcontext *pctx);
typedef CUresult CUDAAPI (*fnCtxSetCurrent)(CUcontext ctx);
typedef CUresult CUDAAPI (*fnCtxDestroy)(CUcontext ctx);
typedef CUresult CUDAAPI (*fnCuLaunchKernel)(CUfunction f,
                                unsigned int gridDimX,
                                unsigned int gridDimY,
                                unsigned int gridDimZ,
                                unsigned int blockDimX,
                                unsigned int blockDimY,
                                unsigned int blockDimZ,
                                unsigned int sharedMemBytes,
                                CUstream hStream,
                                void **kernelParams,
                                void **extra);
/* Interposed function declarations */
extern "C"{
CUresult cuLaunchKernel(CUfunction f,
                                unsigned int gridDimX,
                                unsigned int gridDimY,
                                unsigned int gridDimZ,
                                unsigned int blockDimX,
                                unsigned int blockDimY,
                                unsigned int blockDimZ,
                                unsigned int sharedMemBytes,
                                CUstream hStream,
                                void **kernelParams,
                                void **extra);

/* CUDA Memory management Driver API functions */
CUresult cuMemAlloc(CUdeviceptr* dptr, size_t bytesize);
CUresult cuMemFree(CUdeviceptr dptr);

/* Memcpy H<->D */
CUresult cuMemcpyHtoD ( CUdeviceptr dstDevice, const void* srcHost, size_t ByteCount );
CUresult cuMemcpyHtoDAsync ( CUdeviceptr dstDevice, const void* srcHost, size_t ByteCount, CUstream hStream );
CUresult cuMemcpyDtoH ( void* dstHost, CUdeviceptr srcDevice, size_t ByteCount );
CUresult cuMemcpyDtoHAsync ( void* dstHost, CUdeviceptr srcDevice, size_t ByteCount, CUstream hStream );
} // extern "C"

#endif /* _WRAPCUDA_H_ */
