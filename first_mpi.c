#include <mpi.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv) {
    MPI_Init(NULL, NULL);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int message = 0;

    MPI_Barrier(MPI_COMM_WORLD);

    for (int i = 0; i < size - 1; i++) {
        if (rank == i) {
            // sleep(1);
            message++; 
            printf("I am rank %d & I want to send a message %d\n", rank, message);
            MPI_Send(&message, 1, MPI_INT, i+1, 0, MPI_COMM_WORLD);  
        } else if (rank == i+1){
            printf("I am rank %d & I want to receive a message \n", rank); 
            MPI_Recv(&message, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // sleep(1);
    }

    MPI_Finalize();
    return 0;
}