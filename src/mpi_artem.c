#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "mpi.h"

#define MATRIX_SIZE 2

void printMatrix(double **arr)
{
    for (int i = 0; i < MATRIX_SIZE; i++)
    {
        for (int j = 0; j < MATRIX_SIZE; j++)
        {
            printf("%.2f ", arr[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void fillArray(double **arr)
{
    for (int i = 0; i < MATRIX_SIZE; i++)
    {
        for (int j = 0; j < MATRIX_SIZE; j++)
        {
            int random = rand();
            arr[i][j] = (double)random / RAND_MAX;
        }
    }
}

int main(int argc, char *argv[])
{
    srand(time(0));
    double **matrix_a;
    double **matrix_b;
    double **matrix_c;

    matrix_a = (double **)malloc(MATRIX_SIZE * sizeof(double *));
    matrix_b = (double **)malloc(MATRIX_SIZE * sizeof(double *));
    matrix_c = (double **)malloc(MATRIX_SIZE * sizeof(double *));

    for (int i = 0; i < MATRIX_SIZE; i++)
    {
        matrix_a[i] = (double *)malloc(MATRIX_SIZE * sizeof(double));
        matrix_b[i] = (double *)malloc(MATRIX_SIZE * sizeof(double));
        matrix_c[i] = (double *)malloc(MATRIX_SIZE * sizeof(double));
    }

    int rank;
    int size;
    int dims[3] = {0, 0, 0};    // sinse we are using 3D grid and wnat to calculate the coordinates for each
    int periods[3] = {1, 1, 1}; // no wrapping on the edges
    int coords[3];              // (i,j,k) coordinates of the current process
    int reorder = 1;            // allow reordering of ranks
    MPI_Comm cart_comm, i_comm, j_comm, k_comm;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Dims_create(size, 3, dims);
    // create new communicator in Cartesian Topology
    MPI_Cart_create(MPI_COMM_WORLD, 3, dims, periods, reorder, &cart_comm);
    MPI_Cart_coords(cart_comm, rank, 3, coords); // get coordinates of the process

    double a = 0.00;
    double b = 0.00;
    double cLocal = 0.00;
    double c = 0.00;

    double *buf = NULL;

    if (rank == 0)
    {
        fillArray(matrix_a);
        fillArray(matrix_b);

        printf("Matrix A:\n");
        printMatrix(matrix_a);
        printf("Matrix B:\n");
        printMatrix(matrix_b);

        buf = (double *)malloc(MATRIX_SIZE * MATRIX_SIZE * MATRIX_SIZE * sizeof(double));

        for (int i = 0; i < MATRIX_SIZE; i++)
        {
            for (int j = 0; j < MATRIX_SIZE; j++)
            {
                double value = matrix_a[i][j];
                int dest = i * (MATRIX_SIZE * MATRIX_SIZE) + j * MATRIX_SIZE + j;
                MPI_Send(&value, 1, MPI_DOUBLE, dest, 0, MPI_COMM_WORLD);

                double value2 = matrix_b[i][j];
                int dest2 = i * (MATRIX_SIZE * MATRIX_SIZE) + j * MATRIX_SIZE + i;
                MPI_Send(&value2, 1, MPI_DOUBLE, dest2, 1, MPI_COMM_WORLD);
            }
        }
    }

    int i = coords[0];
    int j = coords[1];
    int k = coords[2];

    if (k == j || (k == 0 && j == 0))
    {
        MPI_Status status;
        MPI_Recv(&a, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &status);
    }
    if (k == i || (k == 0 && i == 0))
    {
        MPI_Status status;
        MPI_Recv(&b, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, &status);
    }

    int i_dims[3] = {0, 1, 0};
    MPI_Cart_sub(cart_comm, i_dims, &i_comm);

    int j_dims[3] = {1, 0, 0};
    MPI_Cart_sub(cart_comm, j_dims, &j_comm);

    MPI_Bcast(&a, 1, MPI_DOUBLE, k, i_comm);
    MPI_Bcast(&b, 1, MPI_DOUBLE, k, j_comm);

    cLocal = a * b;

    int k_dims[3] = {0, 0, 1};
    MPI_Cart_sub(cart_comm, k_dims, &k_comm);

    MPI_Reduce(&cLocal, &c, 1, MPI_DOUBLE, MPI_SUM, 0, k_comm);

    int sub_rank;
    MPI_Comm_rank(k_comm, &sub_rank);

    printf("A %d ( %d , %d , %d ) - %f\n", rank, i, j, k, a);
    printf("B %d ( %d , %d , %d ) - %f\n", rank, i, j, k, b);

    if (sub_rank == 0)
    {
        MPI_Gather(&c, 1, MPI_DOUBLE, buf, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }
    else
    {
        MPI_Gather(&c, 1, MPI_DOUBLE, NULL, 0, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }

    if (rank == 0)
    {
        // for (int o = 0; o < 8; o++)
        // {
        //     printf("%f \n", buf[o]);
        // }
        int count = 0;
        for (int l = 0; l < MATRIX_SIZE; l++)
        {
            for (int m = 0; m < MATRIX_SIZE; m++)
            {
                matrix_c[l][m] = buf[count];
                count += MATRIX_SIZE;
            }
        }
        printf("Matrix C: \n");
        printMatrix(matrix_c);
    }

    MPI_Finalize();

    for (int i = 0; i < MATRIX_SIZE; i++)
    {
        free(matrix_a[i]);
        free(matrix_b[i]);
        free(matrix_c[i]);
    }
    free(matrix_a);
    free(matrix_b);
    free(matrix_c);

    return 0;
}