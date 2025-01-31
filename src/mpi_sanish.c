#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

#define MATRIX_SIZE 4

void print_matrix(double *matrix, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            printf("%6.2f ", matrix[i * size + j]);
        }
        printf("\n");
    }
}

int main(int argc, char **argv) {
    MPI_Comm cannon_comm;
    MPI_Status status;
    int rank, size, dims[2] = {0, 0}, periods[2] = {1, 1};
    int coords[2], my_row, my_col;
    double *A = NULL, *B = NULL, *C_full = NULL;
    double local_A, local_B, local_C;
    double temp;
    double start, end;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Create 2D processor grid
    MPI_Dims_create(size, 2, dims);
    if (dims[0] != dims[1] || dims[0]*dims[1] != size) {
        if (rank == 0) printf("Processors must form square grid matching matrix size!\n");
        MPI_Finalize();
        return 1;
    }
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &cannon_comm);
    MPI_Cart_coords(cannon_comm, rank, 2, coords);
    my_row = coords[0];
    my_col = coords[1];

    // Initialize matrices on root
    if (rank == 0) {
        A = (double *)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(double));
        B = (double *)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(double));
        C_full = (double *)calloc(MATRIX_SIZE * MATRIX_SIZE, sizeof(double));

        srand(time(NULL));
        for(int i = 0; i < MATRIX_SIZE * MATRIX_SIZE; i++) {
            A[i] = (rand() % 11) - 5;
            B[i] = (rand() % 11) - 5;
        }

        printf("Matrix A:\n");
        print_matrix(A, MATRIX_SIZE);
        printf("\nMatrix B:\n");
        print_matrix(B, MATRIX_SIZE);
    }

    // Scatter individual elements
    MPI_Scatter(A, 1, MPI_DOUBLE, &local_A, 1, MPI_DOUBLE, 0, cannon_comm);
    MPI_Scatter(B, 1, MPI_DOUBLE, &local_B, 1, MPI_DOUBLE, 0, cannon_comm);

    // Initial skewing
    int left, right, up, down;
    MPI_Cart_shift(cannon_comm, 1, my_row, &left, &right); // Shift A left by row index
    MPI_Sendrecv(&local_A, 1, MPI_DOUBLE, left, 0, &temp, 1, MPI_DOUBLE, right, 0, cannon_comm, &status);
    local_A = temp;

    MPI_Cart_shift(cannon_comm, 0, my_col, &up, &down); // Shift B up by column index
    MPI_Sendrecv(&local_B, 1, MPI_DOUBLE, up, 0, &temp, 1, MPI_DOUBLE, down, 0, cannon_comm, &status);
    local_B = temp;

    // Cannon's algorithm main loop
    local_C = 0.0;
    start = MPI_Wtime();
    for(int step = 0; step < dims[0]; step++) {
        // Multiply local elements
        local_C += local_A * local_B;

        // Shift A left by 1
        MPI_Cart_shift(cannon_comm, 1, 1, &left, &right);
        MPI_Sendrecv(&local_A, 1, MPI_DOUBLE, left, 0, &temp, 1, MPI_DOUBLE, right, 0, cannon_comm, &status);
        local_A = temp;

        // Shift B up by 1
        MPI_Cart_shift(cannon_comm, 0, 1, &up, &down);
        MPI_Sendrecv(&local_B, 1, MPI_DOUBLE, up, 0, &temp, 1, MPI_DOUBLE, down, 0, cannon_comm, &status);
        local_B = temp;
    }
    end = MPI_Wtime();

    // Gather results
    MPI_Gather(&local_C, 1, MPI_DOUBLE, C_full, 1, MPI_DOUBLE, 0, cannon_comm);

    // Print results
    if (rank == 0) {
        printf("\nResult Matrix C:\n");
        print_matrix(C_full, MATRIX_SIZE);
        printf("\nExecution Time: %.4f seconds\n", end - start);
    }

    // Cleanup
    if (rank == 0) {
        free(A); free(B); free(C_full);
    }
    MPI_Finalize();
    return 0;
}