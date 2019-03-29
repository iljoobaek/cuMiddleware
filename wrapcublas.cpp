#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* We want to interpose using LD_PRELOAD version 2 of cuBLAS API, so 
 * we DON'T include cublas.h
 */
#define CUBLASAPI
#include <cublas_api.h>
#include "wrapcublas.h"
// Note: v2 function declarations are also in cublas_api.h, but they won't bind 
// those in this file

/* This file is for interposing major cublas API functions used within Python ML
 * frameworks like Tensorflow and PyTorch. Intercepting these functions allows
 * our middleware to get to run these functions within its own address-space.
 */

extern "C"
{
	/* TODO: How handle cublas handle creation/destruction */
	cublasStatus_t cublasCreate /*_v2*/ (cublasHandle_t *handle)
	{
		return CUBLAS_STATUS_EXECUTION_FAILED;	
	}
	cublasStatus_t cublasDestroy /*_v2*/ (cublasHandle_t handle)
	{
		return CUBLAS_STATUS_EXECUTION_FAILED;	
	}

	/* cuBLAS interposed functions, no need for real handles */
#define GENERATE_CUBLAS_INTERCEPT(funcname, params, ...)   \
		cublasStatus_t funcname params                                    \
		{                                                                   \
			/* TODO wrap each function? to send to middleware?*/ \
			/*(__VA_ARGS__);*/                          \
			return CUBLAS_STATUS_EXECUTION_FAILED; \
		}

	// Single-precision gemm
	GENERATE_CUBLAS_INTERCEPT(cublasSgemm, (cublasHandle_t handle, 
														  cublasOperation_t transa,
														  cublasOperation_t transb, 
														  int m,
														  int n,
														  int k,
														  const float *alpha, /* host or device pointer */  
														  const float *A, 
														  int lda,
														  const float *B,
														  int ldb, 
														  const float *beta, /* host or device pointer */  
														  float *C,
														  int ldc), 
										 handle, transa, transb, m, n, k, alpha,
										 A, lda, B, ldb, beta, C, ldc);

	// Double-precision gemm
	GENERATE_CUBLAS_INTERCEPT(cublasDgemm, (cublasHandle_t handle, 
														  cublasOperation_t transa,
														  cublasOperation_t transb, 
														  int m,
														  int n,
														  int k,
														  const double *alpha, /* host or device pointer */  
														  const double *A, 
														  int lda,
														  const double *B,
														  int ldb, 
														  const double *beta, /* host or device pointer */  
														  double *C,
														  int ldc), 
										 handle, transa, transb, m, n, k, alpha,
										 A, lda, B, ldb, beta, C, ldc);
	// TODO: Any more gemm functions?
} /* extern "C" */

