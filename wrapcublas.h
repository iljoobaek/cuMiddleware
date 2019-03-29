#ifndef _WRAPCUBLAS_H_
# define _WRAPCUBLAS_H_
#include <cublas_api.h>

/* Interposed cublas functions */
extern "C"
{
/* Note: we actually interpose the "v2" functions of cublas API because
 * these replace the v1 versions at link-time.
 */
cublasStatus_t cublasCreate /*_v2*/ (cublasHandle_t *handle);
cublasStatus_t cublasDestroy /*_v2*/ (cublasHandle_t handle);

cublasStatus_t cublasSgemm /*_v2*/ (cublasHandle_t handle, 
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
                                                      int ldc);
} /* extern "C" */
#endif /* _WRAPCUBLAS_H_ */

