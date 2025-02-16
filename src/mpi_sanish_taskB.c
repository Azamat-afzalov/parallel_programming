#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include <time.h>

#ifndef MATRIX_SIZE
    #define MATRIX_SIZE 800
#endif

void matrix_multiply(double *A, double *B, double *C, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            double sum = 0.0;
            for (int k = 0; k < size; k++) {
                sum += A[i * size + k] * B[k * size + j];
            }
            C[i * size + j] += sum;
        }
    }
}

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
    int coords[2], my_row, my_col, Nl;
    int left, right, up, down;
    double *A = NULL, *B = NULL, *C_full = NULL;
    double *local_A, *local_B, *local_C;
    double *sendbuf_A = NULL, *sendbuf_B = NULL, *recvbuf_C = NULL;
    double start, end;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Create 2D grid
    MPI_Dims_create(size, 2, dims);
    if (dims[0] != dims[1]) {
        if (rank == 0) printf("Processors must form a square grid.\n");
        MPI_Finalize();
        return 1;
    }
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &cannon_comm);
    MPI_Cart_coords(cannon_comm, rank, 2, coords);
    my_row = coords[0];
    my_col = coords[1];
    Nl = MATRIX_SIZE / dims[0]; // Calculates the matrix size each process would get

    // Get neighbor ranks
    MPI_Cart_shift(cannon_comm, 1, 1, &left, &right);  
    MPI_Cart_shift(cannon_comm, 0, 1, &up, &down);     

    // Initialize matrices on root and pack into block-contiguous buffers
    if (rank == 0) {
        A = (double *)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(double));
        B = (double *)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(double));
        C_full = (double *)calloc(MATRIX_SIZE * MATRIX_SIZE, sizeof(double));
        sendbuf_A = (double *)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(double));
        sendbuf_B = (double *)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(double));
        
        srand(time(NULL));
        for(int i = 0; i < MATRIX_SIZE * MATRIX_SIZE; i++) {
            A[i] = (rand() % 11) - 5;  
            B[i] = (rand() % 11) - 5;  
        }

        // printf("Matrix A:\n");
        // print_matrix(A, MATRIX_SIZE);
        // printf("\nMatrix B:\n");
        // print_matrix(B, MATRIX_SIZE);

        // Pack matrices into block-major order
        int idx = 0;
        for (int proc_x = 0; proc_x < dims[0]; proc_x++) {
            for (int proc_y = 0; proc_y < dims[1]; proc_y++) {
                for (int i = 0; i < Nl; i++) {
                    for (int j = 0; j < Nl; j++) {
                        int global_row = proc_x * Nl + i;
                        int global_col = proc_y * Nl + j;
                        sendbuf_A[idx] = A[global_row * MATRIX_SIZE + global_col];
                        sendbuf_B[idx] = B[global_row * MATRIX_SIZE + global_col];
                        idx++;
                    }
                }
            }
        }
    }

    // Allocate local matrices
    local_A = (double *)malloc(Nl * Nl * sizeof(double));
    local_B = (double *)malloc(Nl * Nl * sizeof(double));
    local_C = (double *)calloc(Nl * Nl, sizeof(double));
    double *buf_A = (double *)malloc(Nl * Nl * sizeof(double));
    double *buf_B = (double *)malloc(Nl * Nl * sizeof(double));

    // Add initial distribution of matrices
    MPI_Scatter(sendbuf_A, Nl*Nl, MPI_DOUBLE, local_A, Nl*Nl, MPI_DOUBLE, 0, cannon_comm);
    MPI_Scatter(sendbuf_B, Nl*Nl, MPI_DOUBLE, local_B, Nl*Nl, MPI_DOUBLE, 0, cannon_comm);

    MPI_Request reqs_A[2], reqs_B[2];
    double *tmp;  

    // Initial alignment
    for (int s = 0; s < my_row; s++) {
        MPI_Irecv(buf_A, Nl*Nl, MPI_DOUBLE, right, 0, cannon_comm, &reqs_A[0]);
        MPI_Isend(local_A, Nl*Nl, MPI_DOUBLE, left, 0, cannon_comm, &reqs_A[1]);
        MPI_Waitall(2, reqs_A, MPI_STATUSES_IGNORE);
        tmp = local_A;
        local_A = buf_A;
        buf_A = tmp;
    }

    for (int s = 0; s < my_col; s++) {
        MPI_Irecv(buf_B, Nl*Nl, MPI_DOUBLE, down, 0, cannon_comm, &reqs_B[0]);
        MPI_Isend(local_B, Nl*Nl, MPI_DOUBLE, up, 0, cannon_comm, &reqs_B[1]);
        MPI_Waitall(2, reqs_B, MPI_STATUSES_IGNORE);
        tmp = local_B;
        local_B = buf_B;
        buf_B = tmp;
    }

    start = MPI_Wtime();
    for (int step = 0; step < dims[0]; step++) {
        matrix_multiply(local_A, local_B, local_C, Nl);

        // Shift A left by 1
        MPI_Irecv(buf_A, Nl*Nl, MPI_DOUBLE, right, 0, cannon_comm, &reqs_A[0]);
        MPI_Isend(local_A, Nl*Nl, MPI_DOUBLE, left, 0, cannon_comm, &reqs_A[1]);
        MPI_Waitall(2, reqs_A, MPI_STATUSES_IGNORE);
        tmp = local_A;
        local_A = buf_A;
        buf_A = tmp;

        // Shift B up by 1
        MPI_Irecv(buf_B, Nl*Nl, MPI_DOUBLE, down, 0, cannon_comm, &reqs_B[0]);
        MPI_Isend(local_B, Nl*Nl, MPI_DOUBLE, up, 0, cannon_comm, &reqs_B[1]);
        MPI_Waitall(2, reqs_B, MPI_STATUSES_IGNORE);
        tmp = local_B;
        local_B = buf_B;
        buf_B = tmp;
    }
    end = MPI_Wtime();

    free(buf_A);
    free(buf_B);

    // Gather results using cannon_comm
    if (rank == 0) {
        recvbuf_C = (double *)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(double));
    }
    MPI_Gather(local_C, Nl*Nl, MPI_DOUBLE, recvbuf_C, Nl*Nl, MPI_DOUBLE, 0, cannon_comm);

    // Unpack results into C_full
    if (rank == 0) {
        int idx = 0;
        for (int proc_x = 0; proc_x < dims[0]; proc_x++) {
            for (int proc_y = 0; proc_y < dims[1]; proc_y++) {
                for (int i = 0; i < Nl; i++) {
                    for (int j = 0; j < Nl; j++) {
                        int global_row = proc_x * Nl + i;
                        int global_col = proc_y * Nl + j;
                        C_full[global_row * MATRIX_SIZE + global_col] = recvbuf_C[idx++];
                    }
                }
            }
        }

        printf("\nResult Matrix C:\n");
        print_matrix(C_full, MATRIX_SIZE);
        printf("\nTime: %.4f seconds\n", end - start);
    }

    // Cleanup
    free(local_A); 
    free(local_B); 
    free(local_C);
    if (rank == 0) { 
        free(A); 
        free(B); 
        free(C_full); 
        free(sendbuf_A); 
        free(sendbuf_B); 
        free(recvbuf_C);
    }
    MPI_Finalize();
    return 0;
}