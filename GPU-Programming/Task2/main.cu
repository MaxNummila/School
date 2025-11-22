#include "kernel.cu"
#include "support.h"
#include <stdio.h>

int main(int argc, char **argv) {

  Timer timer;
  cudaError_t cuda_ret;

  // Initialize host variables ----------------------------------------------

  printf("\nSetting up the problem...");
  fflush(stdout);
  startTime(&timer);

  unsigned int n;
  if (argc == 1) {
    n = 10000;
  } else if (argc == 2) {
    n = atoi(argv[1]);
  } else {
    printf("\n    Invalid input parameters!"
           "\n    Usage: ./vecadd               # Vector of size 10,000 is used"
           "\n    Usage: ./vecadd <m>           # Vector of size m is used"
           "\n");
    exit(0);
  }

  float *A_h = (float *)malloc(sizeof(float) * n);
  for (unsigned int i = 0; i < n; i++) {
    A_h[i] = (rand() % 100) / 100.00;
  }

  float *B_h = (float *)malloc(sizeof(float) * n);
  for (unsigned int i = 0; i < n; i++) {
    B_h[i] = (rand() % 100) / 100.00;
  }

  float *C_h = (float *)malloc(sizeof(float) * n);

  stopTime(&timer);
  printf("%f s\n", elapsedTime(timer));
  printf("    Vector size = %u\n", n);

  // Allocate device variables ----------------------------------------------
  float *A_d = NULL;
  float *B_d = NULL;
  float *C_d = NULL;
  size_t size = n * sizeof(float);
  
  printf("Allocating device variables...");
  fflush(stdout);
  startTime(&timer);

  cuda_ret = cudaMalloc((void **)&A_d, size);
  if (cuda_ret != cudaSuccess)
    FATAL("Unable to allocate device memory for A");

  cuda_ret = cudaMalloc((void **)&B_d, size);
  if (cuda_ret != cudaSuccess)
    FATAL("Unable to allocate device memory for B");

  cuda_ret = cudaMalloc((void **)&C_d, size);
  if (cuda_ret != cudaSuccess)
    FATAL("Unable to allocate device memory for C");

  cudaDeviceSynchronize();
  stopTime(&timer);
  printf("%f s\n", elapsedTime(timer));

  // Copy host variables to device ------------------------------------------

  printf("Copying data from host to device...");
  fflush(stdout);
  startTime(&timer);

  cuda_ret = cudaMemcpy(A_d, A_h, size, cudaMemcpyHostToDevice);
  if (cuda_ret != cudaSuccess)
    FATAL("Error with copying A from host to device");
  
    cuda_ret = cudaMemcpy(B_d, B_h, size, cudaMemcpyHostToDevice);
    if (cuda_ret != cudaSuccess)
      FATAL("Error with copying B from host to device");



  cudaDeviceSynchronize();
  stopTime(&timer);
  printf("%f s\n", elapsedTime(timer));

  // Launch kernel ----------------------------------------------------------

  printf("Launching kernel...");
  fflush(stdout);
  startTime(&timer);

  int threadsPerBlock = 256;
  int blocksPerGrid = (n + threadsPerBlock - 1) / threadsPerBlock;

  vecAddKernel<<<blocksPerGrid, threadsPerBlock>>>(A_d, B_d, C_d, n);

  cuda_ret = cudaDeviceSynchronize();
  if (cuda_ret != cudaSuccess)
    FATAL("Unable to launch kernel");
  stopTime(&timer);
  printf("%f s\n", elapsedTime(timer));

  // Copy device variables from host ----------------------------------------

  printf("Copying data from device to host...");
  fflush(stdout);
  startTime(&timer);

  cuda_ret = cudaMemcpy(C_h, C_d, size, cudaMemcpyDeviceToHost);
  if (cuda_ret != cudaSuccess)
    FATAL("Error with copying C from device to host");


  cudaDeviceSynchronize();
  stopTime(&timer);
  printf("%f s\n", elapsedTime(timer));

  // Verify correctness -----------------------------------------------------

  printf("Verifying results...");
  fflush(stdout);

  verify(A_h, B_h, C_h, n);

  // Free memory ------------------------------------------------------------

  free(A_h);
  free(B_h);
  free(C_h);

  cuda_ret = cudaFree(A_d);
  if (cuda_ret != cudaSuccess)
    FATAL("Error with free device memory: A");

  cuda_ret = cudaFree(B_d);
  if (cuda_ret != cudaSuccess)
    FATAL("Error with free device memory: B");

  cuda_ret = cudaFree(C_d);
  if (cuda_ret != cudaSuccess)
    FATAL("Error with free device memory: C");

  return 0;
}
