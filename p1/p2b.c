#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <mpi.h>

void inicializaCadena(char *cadena, int n){
  int i;
  for(i=0; i<n/2; i++){
    cadena[i] = 'A';
  }
  for(i=n/2; i<3*n/4; i++){
    cadena[i] = 'C';
  }
  for(i=3*n/4; i<9*n/10; i++){
    cadena[i] = 'G';
  }
  for(i=9*n/10; i<n; i++){
    cadena[i] = 'T';
  }
}

int MPI_FlattreeColective(void * buff , void * recvbuff , int count ,MPI_Datatype datatype , MPI_Op op , int root , MPI_Comm comm){
    int size,rank,failure,received;
    int *res=recvbuff;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    if(rank==root){
      *res= * (int *) buff;
      for(int i=1;i<size;i++){
        if(failure=MPI_Recv(&received, count, datatype, MPI_ANY_SOURCE, 0, comm, MPI_STATUS_IGNORE)!=MPI_SUCCESS)return failure;
        *res+= received;}
    }
    else{
      if(failure=MPI_Send(buff, count, datatype, root, 0, comm)!=MPI_SUCCESS) return failure;
    }

    return MPI_SUCCESS;
}

int MPI_BinomialBcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
    int size, rank, failure, dest, src;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    for (int i = 1;; i++) {
        if (rank < pow(2, i-1)) {
            dest = rank + pow(2, i-1);
            if (dest < size) {
                if ((failure = MPI_Send(buf, count, datatype, dest, 0, comm)) != MPI_SUCCESS) return failure;
            } 
            else break;
        } 
        else {
          src = rank - pow(2, i-1);
            if (src < pow(2, i-1)) {
                if ((failure = MPI_Recv(buf, count, datatype, rank - pow(2, i - 1), 0, comm, MPI_STATUS_IGNORE)) != MPI_SUCCESS) return failure;
            }
        }
    }
    return MPI_SUCCESS;
}

int main(int argc, char *argv[])
{
  if(argc != 3){
    printf("Numero incorrecto de parametros\nLa sintaxis debe ser: program n L\n  program es el nombre del ejecutable\n  n es el tamaño de la cadena a generar\n  L es la letra de la que se quiere contar apariciones (A, C, G o T)\n");
    exit(1); 
  }
  int numprocs, rank, namelen;
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  int i, n, count=0,res=0;
  char *cadena;
  char L;  

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);


  if(rank == 0){
    n = atoi(argv[1]);
    L = *argv[2];}

    MPI_BinomialBcast(&n,1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_BinomialBcast(&L,1, MPI_CHAR,0, MPI_COMM_WORLD);

    cadena = (char *) malloc(n*sizeof(char));
    inicializaCadena(cadena, n);

  for(i=rank; i<n; i+=numprocs){
    if(cadena[i] == L){
      count++;}}

    MPI_FlattreeColective(&count,&res,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);

  if(rank==0){
  printf("The number of appearances of letter %c is %d\n", L,res);
  }
  MPI_Finalize();
  free(cadena);
  exit(0);
}