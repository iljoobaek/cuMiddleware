// This sample demonstrates a simple library to interpose CUDA symbols

#define __USE_GNU
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <execinfo.h>

#include <vector_types.h> // dim3
#include <driver_types.h> // cudaError_t
#include "wrapcuda.h"


// For interposing dlsym(). See elf/dl-libc.c for the internal dlsym interface function
// For interposing dlopen(). Sell elf/dl-lib.c for the internal dlopen_mode interface function
extern "C" { void* __libc_dlsym (void *map, const char *name); }
extern "C" { void* __libc_dlopen_mode (const char* name, int mode); }

// We need to give the pre-processor a chance to replace a function, such as:
// cuMemAlloc => cuMemAlloc_v2
#define STRINGIFY(x) #x
#define CUDA_SYMBOL_STRING(x) STRINGIFY(x)

// Hack for getting kernel name from CUfunction func
#define HACK_GET_KERN_NAME(func) *(const char **)((uintptr_t)func + 8)

// We need to interpose dlsym since anyone using dlopen+dlsym to get the CUDA driver symbols will bypass
// the hooking mechanism (this includes the CUDA runtime). Its tricky though, since if we replace the
// real dlsym with ours, we can't dlsym() the real dlsym. To get around that, call the 'private'
// libc interface called __libc_dlsym to get the real dlsym.
typedef void* (*fnDlsym)(void*, const char*);

static void* real_dlsym(void *handle, const char* symbol)
{
    static fnDlsym internal_dlsym = (fnDlsym)__libc_dlsym(__libc_dlopen_mode("libdl.so.2", RTLD_LAZY), "dlsym");
    return (*internal_dlsym)(handle, symbol);
}

/* Interposed functions' real function handles */
static fnCuLaunchKernel realCLK_dr;

/*
 ** Interposed Functions
 */
void* dlsym(void *handle, const char *symbol)
{
    // Early out if not a CUDA driver symbol
    if (strncmp(symbol, "cu", 2) != 0) {
        return (real_dlsym(handle, symbol));
    }
	
	if (strcmp(symbol, CUDA_SYMBOL_STRING(cuLaunchKernel)) == 0) {
		if (handle != RTLD_DEFAULT && !realCLK_dr)
		{
			// Presumably, a Python application is trying to locate cuLaunchKernel
			// so hijack the handle to retrieve would-be real fn, if possible
			dlerror();
			const char *error;
			realCLK_dr = (fnCuLaunchKernel)\
				real_dlsym(handle, "cuLaunchKernel");
			// Print error
			if ((error = dlerror()) != NULL) 
			{
				fprintf(stderr, "Hijacking would-be libcuda.so handle \
to retrieve real cuLaunchKernel failed! err: %s\n", error);
				// Should never reach here!
				//exit(EXIT_FAILURE);
			}
		}
		return (void*)(&cuLaunchKernel);
	}
	//else if (strcmp(symbol, CUDA_SYMBOL_STRING(cuMemFree)) == 0) {
	//    return (void*)(&cuMemFree);
	//}
	////else if (strcmp(symbol, CUDA_SYMBOL_STRING(cuCtxGetCurrent)) == 0) {
	////    return (void*)(&cuCtxGetCurrent);
	////}
	//else if (strcmp(symbol, CUDA_SYMBOL_STRING(cuCtxSetCurrent)) == 0) {
	//    return (void*)(&cuCtxSetCurrent);
	//}
	//else if (strcmp(symbol, CUDA_SYMBOL_STRING(cuCtxDestroy)) == 0) {
	//    return (void*)(&cuCtxDestroy);
	//}
    return (real_dlsym(handle, symbol));
}

/* Interposed CUDA Driver API functions */
extern "C" 
{
	CUresult CUDAAPI cuLaunchKernel(CUfunction f,
									unsigned int gridDimX,
									unsigned int gridDimY,
									unsigned int gridDimZ,
									unsigned int blockDimX,
									unsigned int blockDimY,
									unsigned int blockDimZ,
									unsigned int sharedMemBytes,
									CUstream hStream,
									void **kernelParams,
									void **extra)
	{
		fprintf(stderr, "Caught my cuLaunchKernel (Driver API)!!\n");

		if (!realCLK_dr)
		{
			// If caught dlsym call did not find realCLK_dr from caught 
			// manual-binding from within Python (presumably), then only chance 
			// left is to look for the real cuLaunchKernel symbol ourselves at 
			// runtime (only case should from a C++ application)
			const char *error;

			// Clear dlerror 
			dlerror();

			// Find CUDA Driver/C API's cuLaunchKernel definition
			// using real_dlsym. 
			realCLK_dr = (fnCuLaunchKernel)real_dlsym(RTLD_NEXT,
												 CUDA_SYMBOL_STRING("cuLaunchKernel"));

			fprintf(stderr, "realCLK_dr set to %p\n", (void*)realCLK_dr);
			// Print error
			if ((error = dlerror()) != NULL) 
			{
				fprintf(stderr, "%s\n", error);
				exit(EXIT_FAILURE);
			}
		}

		/*
		 * Package kernel, launch config, and kernel args for server
		 */
		// First, get the kernel symbol name using hack from
		// https://devtalk.nvidia.com/default/topic/821920/how-to-get-a-kernel-functions-name-through-its-pointer-/
		const char *kern_name = HACK_GET_KERN_NAME(f);	
		fprintf(stderr, "Retrieved kernel name from CUfunction f, %s\n", kern_name);

		return realCLK_dr(f, 
							gridDimX, gridDimY, gridDimZ,
							blockDimX, blockDimY, blockDimZ,
							sharedMemBytes, hStream, kernelParams,
							extra);
	}
} /* extern "C" */

