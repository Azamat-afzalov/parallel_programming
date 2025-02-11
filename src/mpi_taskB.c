#include <mpi.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define MATRIX_SIZE 4   
#define GRID_SIZE 2     
#define BLOCK_SIZE (MATRIX_SIZE/GRID_SIZE)

void print_matrix(const int *matrix, int size, const char *matrix_name) {
    printf("%s:\n", matrix_name);
    for (int i = 0; i < size; i++) {
        printf("%d ", matrix[i]);
        if ((i + 1) % MATRIX_SIZE == 0) {
            printf("\n");
        }
    }
    printf("\n");
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

    // Create 2D Cartesian topology
    int dims[2] = {GRID_SIZE, GRID_SIZE};
    int periods[2] = {1, 1};
    int coords[2];
    MPI_Comm cart_comm;
    
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &cart_comm);
    MPI_Cart_coords(cart_comm, rank, 2, coords);

    // Allocate memory for local blocks
    int block_elements = BLOCK_SIZE * BLOCK_SIZE;
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
            // A[i] = rand() % 10;
            // B[i] = rand() % 10;
            A[i] = i;
            B[i] = i;
        }

        print_matrix(A, MATRIX_SIZE * MATRIX_SIZE, "Matrix A");
        print_matrix(B, MATRIX_SIZE * MATRIX_SIZE, "Matrix B");
    }

    // Create data type for matrix blocks
    MPI_Datatype block_type;
    MPI_Datatype temp_type;
    
    MPI_Type_vector(BLOCK_SIZE, BLOCK_SIZE, MATRIX_SIZE, MPI_INT, &temp_type);
    MPI_Type_create_resized(temp_type, 0, sizeof(int), &block_type);
    MPI_Type_commit(&block_type);

    // Calculate send counts and displacements for scattering blocks
    int *sendcounts = NULL, *displs = NULL;
    if (rank == 0) {
        sendcounts = (int *)malloc(size * sizeof(int));
        displs = (int *)malloc(size * sizeof(int));
        
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                sendcounts[i * GRID_SIZE + j] = 1;
                displs[i * GRID_SIZE + j] = i * MATRIX_SIZE * BLOCK_SIZE + j * BLOCK_SIZE;
            }
        }
    }

    // Scatter blocks of matrices A and B
    MPI_Scatterv(A, sendcounts, displs, block_type,
                 local_A, block_elements, MPI_INT,
                 0, cart_comm);
    MPI_Scatterv(B, sendcounts, displs, block_type,
                 local_B, block_elements, MPI_INT,
                 0, cart_comm);

    // Create row communicator
    MPI_Comm row_comm;
    MPI_Comm_split(cart_comm, coords[0], coords[1], &row_comm);

    // Main Fox algorithm
    for (int stage = 0; stage < GRID_SIZE; stage++) {
        int bcast_coord = (coords[0] + stage) % GRID_SIZE;
        
        // Broadcast A blocks along rows
        if (coords[1] == bcast_coord) {
            memcpy(temp_A, local_A, block_elements * sizeof(int));
        }
        MPI_Bcast(temp_A, block_elements, MPI_INT, bcast_coord, row_comm);

        // Multiply blocks
        multiply_blocks(temp_A, local_B, local_C, BLOCK_SIZE);

        // Shift B blocks up by one
        int src, dest;
        MPI_Cart_shift(cart_comm, 0, -1, &src, &dest);
        MPI_Sendrecv_replace(local_B, block_elements, MPI_INT,
                            dest, 0, src, 0,
                            cart_comm, MPI_STATUS_IGNORE);
    }

    // Gather results
    MPI_Gatherv(local_C, block_elements, MPI_INT,
                C, sendcounts, displs, block_type,
                0, cart_comm);

    // Print result
    if (rank == 0) {
        print_matrix(C, MATRIX_SIZE * MATRIX_SIZE, "Result Matrix C");
        
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