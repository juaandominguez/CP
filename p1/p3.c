#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>

#define DEBUG 0

/* Translation of the DNA bases
   A -> 0
   C -> 1
   G -> 2
   T -> 3
   N -> 4*/

#define M  1000000 // Number of sequences
#define N  200  // Number of bases per sequence

unsigned int g_seed = 0;

int fast_rand(void) {
    g_seed = (214013*g_seed+2531011);
    return (g_seed>>16) % 5;
}

// The distance between two bases
int base_distance(int base1, int base2){

  if((base1 == 4) || (base2 == 4)){
    return 3;
  }

  if(base1 == base2) {
    return 0;
  }

  if((base1 == 0) && (base2 == 3)) {
    return 1;
  }

  if((base2 == 0) && (base1 == 3)) {
    return 1;
  }

  if((base1 == 1) && (base2 == 2)) {
    return 1;
  }

  if((base2 == 2) && (base1 == 1)) {
    return 1;
  }

  return 2;
}

int main(int argc, char *argv[] ) {

int i, j;
int *data1, *data2, *div1, *div2;
int *result,*partial_res;
struct timeval  tv1, tv2, tv3, tv4;

int size, rank,rows;

MPI_Init(&argc,&argv); 

MPI_Comm_rank(MPI_COMM_WORLD, &rank);
MPI_Comm_size(MPI_COMM_WORLD, &size);



rows=M/size + (M %size ? 1:0); 

if(rank==0){

    data1 = (int *) malloc(rows*size*N*sizeof(int));
    data2 = (int *) malloc(rows*size*N*sizeof(int));
    result = (int *) malloc(rows*size*sizeof(int));

    /* Initialize Matrices */
    for(i=0;i<M;i++) {
        for(j=0;j<N;j++) {
            /* random with 20% gap proportion */
            data1[i*N+j] = fast_rand();
            data2[i*N+j] = fast_rand();
        }
    }
}

div1=(int *) malloc(rows*N*sizeof(int));
div2=(int *) malloc(rows*N*sizeof(int));    
partial_res=(int *) malloc(rows*sizeof(int));

gettimeofday(&tv1, NULL);

MPI_Scatter(data1,rows*N,MPI_INT,div1,rows*N,MPI_INT,0,MPI_COMM_WORLD);
MPI_Scatter(data2,rows*N,MPI_INT,div2,rows*N,MPI_INT,0,MPI_COMM_WORLD);

gettimeofday(&tv2, NULL);
int communication = (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);

int block_size;

if( rank == size-1){
  block_size=M-rows*(size-1);
}else block_size=rows;

gettimeofday(&tv3, NULL);
for(i=0;i<block_size;i++) {
    partial_res[i]=0;
    for(j=0;j<N;j++) {
        partial_res[i] += base_distance(div1[i*N+j], div2[i*N+j]);
    }
}
gettimeofday(&tv4, NULL);
int computation = (tv4.tv_usec - tv3.tv_usec)+ 1000000 * (tv4.tv_sec - tv3.tv_sec);

gettimeofday(&tv1, NULL);

MPI_Gather(partial_res,block_size,MPI_INT,result,block_size,MPI_INT,0,MPI_COMM_WORLD);

gettimeofday(&tv2, NULL);

communication+=(tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);


    


  /* Display result */
  if (DEBUG == 1 && rank==0) {
    int checksum = 0;
    for(i=0;i<M;i++) {
      checksum += result[i];
    }
    printf("Checksum: %d\n ", checksum);
  } else if (DEBUG == 2 && rank ==0) {
    for(i=0;i<M;i++) {
      printf(" %d \t ",partial_res[i]);
    }
  } else if (DEBUG == 0) {
    printf ("I am process %d, and I spent on Communication: Time (seconds) = %lf, and on Computation: Time (seconds) = %lf\n",rank, (double) communication/1E6,(double) computation/1E6);
  }    


  if(rank==0){
    free(data1); free(data2); free(result);
  }

  free(div1);
  free(div2);
  free(partial_res);


  MPI_Finalize();

  return 0;
}
