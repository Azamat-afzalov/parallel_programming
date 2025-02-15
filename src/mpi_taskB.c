#include <mpi.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
// Matrix Size  Num Process   Grid size
// 800           64              100
// 798           361             42
// 2000          64              250
// 1995          361   
// 4000          64              500
// 4009          361
// 6000          64              750
// 6004          361  
// 7995          225
// 7999          361

#ifndef MATRIX_SIZE
    #define MATRIX_SIZE 800
#endif
#ifndef PRINT
    #define PRINT false
#endif

void print_matrix(const int *matrix, int size, const char *matrix_name) {
    if (PRINT) {
        printf("%s:\n", matrix_name);
        for (int i = 0; i < size; i++) {
            printf("%d ", matrix[i]);
            if ((i + 1) % MATRIX_SIZE == 0) {
                printf("\n");
            }
        }
        printf("\n");
    }
   
}

// Function to multiply two block matrices
void multiply_blocks(int *A, int *B, int *C, int block_size) {
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            for (int k = 0; k < block_size; k++) {
                C[i * block_size + j] += 
                    A[i * block_size + k] * B[k * block_size + j];
            }
        }
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int grid_size = (int)sqrt(size); // sqrt of number of processes
    int block_size = MATRIX_SIZE/grid_size;

    // Create 2D Cartesian topology
    int dims[2] = {grid_size, grid_size};
    int periods[2] = {1, 1};
    int coords[2];
    MPI_Comm cart_comm;
    
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &cart_comm);
    MPI_Cart_coords(cart_comm, rank, 2, coords);

    int block_elements = block_size * block_size;
    int *local_A = (int *)calloc(block_elements, sizeof(int));
    int *local_B = (int *)calloc(block_elements, sizeof(int));
    int *local_C = (int *)calloc(block_elements, sizeof(int));
    int *temp_A = (int *)calloc(block_elements, sizeof(int));

    // Root process initializes matrices
    int *A = NULL, *B = NULL, *C = NULL;
    if (rank == 0) {
        A = (int *)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(int));
        B = (int *)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(int));
        C = (int *)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(int));

        // Initialize matrices with random values
        srand(time(NULL));
        for (int i = 0; i < MATRIX_SIZE * MATRIX_SIZE; i++) {
            A[i] = rand() % 10;
            B[i] = rand() % 10;
            // A[i] = i;
            // B[i] = i;
        }

        print_matrix(A, MATRIX_SIZE * MATRIX_SIZE, "Matrix A");
        print_matrix(B, MATRIX_SIZE * MATRIX_SIZE, "Matrix B");
    }

    // Create data type for matrix blocks
    MPI_Datatype block_type;
    MPI_Datatype temp_type;
    
    MPI_Type_vector(block_size, block_size, MATRIX_SIZE, MPI_INT, &temp_type);
    MPI_Type_create_resized(temp_type, 0, sizeof(int), &block_type);
    MPI_Type_commit(&block_type);

    double startTime = MPI_Wtime();

    // Calculate send counts and displacements for scattering blocks
    int *sendcounts = NULL, *displs = NULL;
    if (rank == 0) {
        sendcounts = (int *)malloc(size * sizeof(int));
        displs = (int *)malloc(size * sizeof(int));
        
        for (int i = 0; i < grid_size; i++) {
            for (int j = 0; j < grid_size; j++) {
                sendcounts[i * grid_size + j] = 1;
                displs[i * grid_size + j] = i * MATRIX_SIZE * block_size + j * block_size;
            }
        }
    }

    MPI_Scatterv(A, sendcounts, displs, block_type, local_A, block_elements, MPI_INT, 0, cart_comm);
    MPI_Scatterv(B, sendcounts, displs, block_type, local_B, block_elements, MPI_INT, 0, cart_comm);

    // Create row communicator
    MPI_Comm row_comm;
    MPI_Comm_split(cart_comm, coords[0], coords[1], &row_comm);

    // Main alghoritm code
    for (int stage = 0; stage < grid_size; stage++) {
        int bcast_coord = (coords[0] + stage) % grid_size;
        
        // Broadcast A blocks along rows
        if (coords[1] == bcast_coord) {
            memcpy(temp_A, local_A, block_elements * sizeof(int));
        }
        MPI_Bcast(temp_A, block_elements, MPI_INT, bcast_coord, row_comm);

        // Multiply blocks
        multiply_blocks(temp_A, local_B, local_C, block_size);

        // Shift B blocks up by one
        int src, dest;
        MPI_Cart_shift(cart_comm, 0, -1, &src, &dest);
        MPI_Sendrecv_replace(local_B, block_elements, MPI_INT, dest, 0, src, 0, cart_comm, MPI_STATUS_IGNORE);
    }

    // Gather results
    MPI_Gatherv(local_C, block_elements, MPI_INT, C, sendcounts, displs, block_type, 0, cart_comm);
    double endTime = MPI_Wtime();

    // Print result
    if (rank == 0) {
        print_matrix(C, MATRIX_SIZE * MATRIX_SIZE, "Result Matrix C");
        printf("Total time: %.2lf ", endTime - startTime);
        free(A);
        free(B);
        free(C);
        free(sendcounts);
        free(displs);
    }

    // Cleanup
    free(local_A);
    free(local_B);
    free(local_C);
    free(temp_A);
    MPI_Type_free(&block_type);
    MPI_Comm_free(&row_comm);
    MPI_Comm_free(&cart_comm);
    MPI_Finalize();
    return 0;
}