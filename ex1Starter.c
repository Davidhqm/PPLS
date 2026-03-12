// PPLS Exercise 1 Starter File
//
// See the exercise sheet for details
//
// Note that NITEMS, NTHREADS and SHOWDATA should
// be defined at compile time with -D options to gcc.
// They are the array length to use, number of threads to use
// and whether or not to printout array contents (which is
// useful for debugging, but not a good idea for large arrays).

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h> // in case you want semaphores, but you don't really
                       // need them for this exercise


// Print a helpful message followed by the contents of an array
// Controlled by the value of SHOWDATA, which should be defined
// at compile time. Useful for debugging.
void showdata (char *message,  int *data,  int n) {
  int i; 

if (SHOWDATA) {
    printf ("%s", message);
    for (i=0; i<n; i++ ){
     printf (" %d", data[i]);
    }
    printf("\n");
  }
}

// Check that the contents of two integer arrays of the same length are equal
// and return a C-style boolean
int checkresult (int* correctresult,  int *data,  int n) {
  int i; 

  for (i=0; i<n; i++ ){
    if (data[i] != correctresult[i]) return 0;
  }
  return 1;
}

// Compute the prefix sum of an array **in place** sequentially
void sequentialprefixsum (int *data, int n) {
  int i;

  for (i=1; i<n; i++ ) {
    data[i] = data[i] + data[i-1];
  }
}

// Worker thread arguments structure to bundle the parameters we need to pass to the worker threads.
typedef struct {
  int tid;
  int n; // total number of items in the array
  int p; // number of threads
  int base;
  int *data;
  pthread_barrier_t *barrier;
} worker_args;

static int chunk_end_index(int tid, int p, int n, int base) {
  if (tid == p - 1) return n - 1; //  index for final chunck  
  return ((tid + 1) * base) - 1; // normal chunk end index for tid < p-1
}

static void *prefix_worker(void *arg) {
  worker_args *a = (worker_args *)arg;
  int start = a->tid * a->base;
  int end = chunk_end_index(a->tid, a->p, a->n, a->base);
  int i, t; 

  // Phase 1: in-place prefix sum within the chunk
  for (i = start + 1; i <= end; i++) a->data[i] += a->data[i - 1];
  pthread_barrier_wait(a->barrier);

  // Phase 2: only thread 0 updates the chunk-top elements with a prefix sum over those tops
  if (a->tid == 0) {
    int prev_end = chunk_end_index(0, a->p, a->n, a->base);
    for (t = 1; t < a->p; t++) {
      int top = chunk_end_index(t, a->p, a->n, a->base);
      a->data[top] += a->data[prev_end];
      prev_end = top;
    }
  }
  pthread_barrier_wait(a->barrier);

  // Phase 3: threads 1..p-1 add the previous chunk's final value to all elements in their chunk except final element
  if (a->tid > 0) {
    int offset = a->data[chunk_end_index(a->tid - 1, a->p, a->n, a->base)];
    for (i = start; i < end; i++) a->data[i] += offset;
  }
  pthread_barrier_wait(a->barrier);
  return NULL;
}


// YOU MUST WRITE THIS FUNCTION AND ANY ADDITIONAL FUNCTIONS YOU NEED
void parallelprefixsum (int *data, int n) {
  pthread_t threads[NTHREADS];
  worker_args args[NTHREADS];
  pthread_barrier_t barrier;
  int t, base;

  /*
    1) Create NTHREADS worker threads once, each owning one contiguous chunk.
       Chunk size is floor(n/NTHREADS) for all but the final chunk, which absorbs
       any remainder items so all n elements are covered.
    2) Phase 1: every worker performs an in-place sequential prefix sum inside
       its own chunk only.
    3) Phase 2: after a barrier, only worker 0 updates the chunk-top elements
       (the last index of each chunk) with a sequential prefix over those tops.
       Other workers wait at the same barrier points.
    4) Phase 3: after another barrier, workers 1 to NTHREADS-1 add the previous
       chunk's final value to all elements in their chunk but their own final
       element.
    Barriers are used only between phases to enforce required data dependencies.
  */

  if (n <= 1) return;
  if (NTHREADS <= 1 || n < NTHREADS) {
    sequentialprefixsum(data, n);
    return;
  }

  base = n / NTHREADS;
  pthread_barrier_init(&barrier, NULL, NTHREADS);

  for (t = 0; t < NTHREADS; t++) {
    args[t].tid = t;
    args[t].n = n;
    args[t].p = NTHREADS;
    args[t].base = base;
    args[t].data = data;
    args[t].barrier = &barrier;
    pthread_create(&threads[t], NULL, prefix_worker, &args[t]);   
  }
  for (t = 0; t < NTHREADS; t++) pthread_join(threads[t], NULL);

  pthread_barrier_destroy(&barrier);
}


int main (int argc, char* argv[]) {

  int *arr1, *arr2, i;

  // Check that the compile time constants are sensible
  if ((NITEMS>10000000) || (NTHREADS>32)) {
    printf ("So much data or so many threads may not be a good idea! .... exiting\n");
    exit(EXIT_FAILURE);
  }

  // Create two copies of some random data
  arr1 = (int *) malloc(NITEMS*sizeof(int));
  arr2 = (int *) malloc(NITEMS*sizeof(int));
  srand((int)time(NULL));
  for (i=0; i<NITEMS; i++) {
     arr1[i] = arr2[i] = rand()%5;
  }
  showdata ("initial data          : ", arr1, NITEMS);

  // Calculate prefix sum sequentially, to check against later on
  sequentialprefixsum (arr1, NITEMS);
  showdata ("sequential prefix sum : ", arr1, NITEMS);

  // Calculate prefix sum in parallel on the other copy of the original data
  parallelprefixsum (arr2, NITEMS);
  showdata ("parallel prefix sum   : ", arr2, NITEMS);

  // Check that the sequential and parallel results match
  if (checkresult(arr1, arr2, NITEMS))  {
    printf("Well done, the sequential and parallel prefix sum arrays match.\n");
  } else {
    printf("Error: The sequential and parallel prefix sum arrays don't match.\n");
  }

  free(arr1); free(arr2);
  return 0;
}
