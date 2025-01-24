#include<mpi.h>
#include<stdio.h>
#include<math.h>
#include <time.h>
#include <stdlib.h>

void print_matrix(const int *matrix, int size, int matrix_size, const char *matrix_name) {
    printf("Generated %s:\n", matrix_name);
    for (int i = 0; i < size; i++) {
        printf("%d ", matrix[i]);
        if (i % matrix_size == matrix_size - 1) {
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

    int matrix_size = (int)sqrt(size);

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
    
    int A[size];
    int B[size];   

    if (rank == 0) {
        
        for (int i = 0; i < size; i++) {
            // A[i] = i + 1;
            // B[i] = i + 1; 
            A[i] = rand() % 10;
            B[i] = rand() % 10;
            // C[i] = 0;
            // MPI_Send(&A[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD);  // tag 0 -> A[i][j]
            // MPI_Send(&B[i], 1, MPI_INT, i, 1, MPI_COMM_WORLD);  // tag 1 -> B[i][j]
        }
        

        print_matrix(A, size, matrix_size, "A");
        print_matrix(B, size, matrix_size, "B");

    }

        // int received_a;
        // int received_b;

        //  MPI_Barrier(MPI_COMM_WORLD);

        // printf("Rank %d received A[%d][%d] received %d \n",rank, i, j, received_a);
        // printf("Rank %d received B[%d][%d] received %d \n",rank, i, j, received_b);
    

        // MPI_Recv(&received_a, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // Received A
        // MPI_Recv(&received_b, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // Received B

        // printf("Rank %d received A[%d][%d] received %d \n",rank, i, j, received_a);
        // printf("Rank %d received B[%d][%d] received %d \n",rank, i, j, received_b);



    MPI_Scatter(A, 1, MPI_INT, &received_a, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(B, 1, MPI_INT, &received_b, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // printf("Rank %d received A[%d][%d] received %d \n",rank, i, j, received_a);
    // printf("Rank %d received B[%d][%d] received %d \n",rank, i, j, received_b);
    // printf("Rank: %d, Cords: (%d, %d)", rank, coords[0], coords[1]);

    MPI_Comm row_comm;

    MPI_Comm_split(cart_comm, coords[0], rank, &row_comm);

    int C = 0;

    for (int i = 0; i < matrix_size; i++) {
        int diagonal = -1;
        int bcast_row = (coords[0] + i) % matrix_size;

        // printf("BCAST_R=%d, i=%d, cors0=%d, cords1=%d,  \t", bcast_row, i, coords[0], coords[1]);

        if (bcast_row == coords[1]) {
            diagonal = received_a;
            // printf("Diagonal element: A[%d][%d] = %d\n", coords[0], coords[1], received_a);
        }

        MPI_Bcast(&diagonal, 1, MPI_INT, bcast_row, row_comm);
        
        if (diagonal != -1) {
            // printf("D: %d, B: %d, d*B=%d\n", diagonal, received_b,  diagonal*received_b);
            // printf("comm rank: %d, %d \n", comm_rank, C[comm_rank]);
            C += diagonal * received_b;
            // printf("C[%d]=%d \n", comm_rank, C[comm_rank]);
        }

        // printf("D=%d, B=%d, C=%d, \n",diagonal, received_b, C);
        int src, dest;
        MPI_Cart_shift(cart_comm, 0, -1, &src, &dest);


        MPI_Sendrecv_replace(&received_b, 1, MPI_INT, dest, 0, src, 0, cart_comm, MPI_STATUS_IGNORE);

        // MPI_Barrier(cart_comm);
    }
    
    
    int res[size];
    MPI_Gather(&C, 1, MPI_INT, res, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        // printf("Final Result:\n");
        print_matrix(res, size, matrix_size, "C");
        // printf("\n\n\n\n");
    }
    
    // printf("Received Diagonal=%d, for row B=%d \n", diagonal, received_b);
    free(res);
    free(A);
    free(B);
    MPI_Comm_free(&row_comm);
    MPI_Comm_free(&cart_comm);
    MPI_Finalize();
    return 0;
}
