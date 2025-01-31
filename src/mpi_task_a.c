// Afzalov Azamat, MPI task A
#include<mpi.h>
#include<stdio.h>
#include<math.h>
#include <time.h>
#include <stdlib.h>

#define n 3 // n^2 = p

// print_matrix() function is craeted by using LLMs
void print_matrix(const int *matrix, int size, const char *matrix_name) {
    printf("%s:\n", matrix_name);
    for (int i = 0; i < size; i++) {
        printf("%d ", matrix[i]);
        if (i % n == n - 1) {
            printf("\n");
        }
    }
    printf("\n");
}

int main(int argc, char **argv) {

    MPI_Init(&argc, &argv);

    int rank, size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

   

    int num_of_dimensions = 2;                    
    int dims[2] = {0, 0}; // This will be {2,2} with 4 processes
    int periods[2] = {1, 1};  // Periodic will wrap the edges of matrix  
    int reorder = 0;
    int coords[2]; 
    int comm_rank;

    MPI_Comm cart_comm; // Cartesian topology

    MPI_Dims_create(size, num_of_dimensions, dims);
    MPI_Cart_create(MPI_COMM_WORLD, num_of_dimensions, dims, periods, 1, &cart_comm);
    MPI_Cart_coords(cart_comm, rank, num_of_dimensions, coords);
    MPI_Cart_rank(cart_comm, coords, &comm_rank);

    srand(time(NULL) + rank);


    int received_a;
    int received_b;
    
    int A[n * n];
    int B[n * n];   

    if (rank == 0) {
        
        for (int i = 0; i < size; i++) {
            A[i] = rand() % 10;
            B[i] = rand() % 10;
        }

        print_matrix(A, n * n, "Generated A");
        print_matrix(B, n * n, "Generated B");

    }

    MPI_Scatter(A, 1, MPI_INT, &received_a, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(B, 1, MPI_INT, &received_b, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // printf("Rank %d received A[%d][%d] received %d \n",rank, i, j, received_a);
    // printf("Rank %d received B[%d][%d] received %d \n",rank, i, j, received_b);
    // printf("Rank: %d, Cords: (%d, %d)", rank, coords[0], coords[1]);

    MPI_Comm row_comm;

    MPI_Comm_split(cart_comm, coords[0], rank, &row_comm);

    int C = 0;

    for (int i = 0; i < n; i++) {
        int diagonal = -1;
        int bcast_row = (coords[0] + i) % n;

        if (bcast_row == coords[1]) {
            diagonal = received_a;
        }

        MPI_Bcast(&diagonal, 1, MPI_INT, bcast_row, row_comm);
        
        if (diagonal != -1) {
            C += diagonal * received_b;
        }

        int src, dest;
        MPI_Cart_shift(cart_comm, 0, -1, &src, &dest);


        MPI_Sendrecv_replace(&received_b, 1, MPI_INT, dest, 0, src, 0, cart_comm, MPI_STATUS_IGNORE);

    }
    
    
    int res[n * n];
    MPI_Gather(&C, 1, MPI_INT, res, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        print_matrix(res, size, "Calculated result");
    }


    MPI_Comm_free(&row_comm);
    MPI_Comm_free(&cart_comm);
    MPI_Finalize();
    return 0;
}
