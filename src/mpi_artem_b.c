#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "mpi.h"

#ifndef MATRIX_SIZE
#define MATRIX_SIZE 1000
#endif

#ifndef PROC
#define PROC 2
#endif

void subMatrixMultiply(int *A, int *B, int *C)
{
    int sub = MATRIX_SIZE / PROC;
    for (int i = 0; i < sub; i++)
    {
        for (int j = 0; j < sub; j++)
        {
            C[i * sub + j] = 0;
            for (int k = 0; k < sub; k++)
            {
                C[i * sub + j] += A[i * sub + k] * B[k * sub + j];
            }
        }
    }
}

void printMatrix(int *arr, int size)
{
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            printf("%d ", arr[(i * size) + j]);
        }
        printf("\n");
    }
    printf("\n");
}

void fillArray(int *arr)
{
    for (int i = 0; i < MATRIX_SIZE * MATRIX_SIZE; i++)
    {
        arr[i] = rand() % 11;
    }
}

int main(int argc, char *argv[])
{

    srand(time(0));
    int *matrix_a;
    int *matrix_b;
    int *matrix_c;
    int *temp_a;
    int *temp_b;

    int sub_size = MATRIX_SIZE / PROC;

    int rank;
    int size;
    int dims[3] = {0, 0, 0};    // sinse we are using 3D grid and wnat to calculate the coordinates for each
    int periods[3] = {1, 1, 1}; // no wrapping on the edges
    int coords[3];              // (i,j,k) coordinates of the current process
    int reorder = 1;            // allow reordering of ranks

    MPI_Init(&argc, &argv);

    double startTime = MPI_Wtime();

    MPI_Comm cart_comm, i_comm, j_comm, k_comm;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Dims_create(size, 3, dims);
    // create new communicator in Cartesian Topology
    MPI_Cart_create(MPI_COMM_WORLD, 3, dims, periods, reorder, &cart_comm);
    MPI_Cart_coords(cart_comm, rank, 3, coords); // get coordinates of the process

    int *sub_a = (int *)malloc(sub_size * sub_size * sizeof(int));
    int *sub_b = (int *)malloc(sub_size * sub_size * sizeof(int));
    int *sub_c = (int *)malloc(sub_size * sub_size * sizeof(int));
    int *c = (int *)malloc(sub_size * sub_size * sizeof(int));
    int *buf = (int *)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(int));

    int *ranks_a = (int *)malloc(PROC * PROC * sizeof(int));
    int *ranks_b = (int *)malloc(PROC * PROC * sizeof(int));

    int ranksCounter = 0;
    for (int i = 0; i < PROC; i++)
    {
        for (int j = 0; j < PROC; j++)
        {
            ranks_a[ranksCounter] = i * (PROC * PROC) + j * PROC + j;
            ranks_b[ranksCounter] = i * (PROC * PROC) + j * PROC + i;
            ranksCounter++;
        }
    }

    double startArrFill, endArrFill;

    if (rank == 0)
    {
        matrix_a = (int *)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(int));
        matrix_b = (int *)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(int));
        matrix_c = (int *)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(int));
        temp_a = (int *)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(int));
        temp_b = (int *)malloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(int));

        startArrFill = MPI_Wtime();

        fillArray(matrix_a);
        fillArray(matrix_b);

        endArrFill = MPI_Wtime();

        // printf("Matrix A:\n");
        // printMatrix(matrix_a, MATRIX_SIZE);
        // printf("Matrix B:\n");
        // printMatrix(matrix_b, MATRIX_SIZE);

        int counter = 0;
        for (int i = 0; i < PROC; i++)
        {
            for (int j = 0; j < PROC; j++)
            {
                for (int x = 0; x < sub_size; x++)
                {
                    for (int y = 0; y < sub_size; y++)
                    {
                        temp_a[counter] = matrix_a[(i * sub_size * MATRIX_SIZE) + (j * sub_size) + (x * MATRIX_SIZE) + y];
                        temp_b[counter] = matrix_b[(i * sub_size * MATRIX_SIZE) + (j * sub_size) + (x * MATRIX_SIZE) + y];
                        counter++;
                    }
                }
            }
        }

        // printf("Temp A:\n");
        // printMatrix(temp_a, MATRIX_SIZE);
        // printf("Temp B:\n");
        // printMatrix(temp_b, MATRIX_SIZE);
    }

    double startScatter = MPI_Wtime();

    MPI_Group world_group;
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);

    MPI_Group ranks_for_a, ranks_for_b;
    MPI_Group_incl(world_group, PROC * PROC, ranks_a, &ranks_for_a);
    MPI_Group_incl(world_group, PROC * PROC, ranks_b, &ranks_for_b);

    MPI_Comm comm_a, comm_b;
    MPI_Comm_create(MPI_COMM_WORLD, ranks_for_a, &comm_a);
    MPI_Comm_create(MPI_COMM_WORLD, ranks_for_b, &comm_b);

    if (comm_a != MPI_COMM_NULL)
    {
        MPI_Scatter(temp_a, sub_size * sub_size, MPI_INT, sub_a, sub_size * sub_size, MPI_INT, 0, comm_a);
    }
    if (comm_b != MPI_COMM_NULL)
    {
        MPI_Scatter(temp_b, sub_size * sub_size, MPI_INT, sub_b, sub_size * sub_size, MPI_INT, 0, comm_b);
    }

    double endScatter = MPI_Wtime();

    // int i = coords[0];
    // int j = coords[1];
    int k = coords[2];

    double startBcast = MPI_Wtime();

    int i_dims[3] = {0, 1, 0};
    MPI_Cart_sub(cart_comm, i_dims, &i_comm);

    int j_dims[3] = {1, 0, 0};
    MPI_Cart_sub(cart_comm, j_dims, &j_comm);

    MPI_Bcast(sub_a, sub_size * sub_size, MPI_INT, k, i_comm);
    MPI_Bcast(sub_b, sub_size * sub_size, MPI_INT, k, j_comm);

    double endBcast = MPI_Wtime();

    subMatrixMultiply(sub_a, sub_b, sub_c);

    double endMatrixMultipy = MPI_Wtime();

    int k_dims[3] = {0, 0, 1};
    MPI_Cart_sub(cart_comm, k_dims, &k_comm);

    MPI_Reduce(sub_c, c, sub_size * sub_size, MPI_INT, MPI_SUM, 0, k_comm);

    double endReduce = MPI_Wtime();

    int sub_rank;
    MPI_Comm_rank(k_comm, &sub_rank);

    MPI_Comm gather_comm;
    MPI_Comm_split(MPI_COMM_WORLD, sub_rank, 0, &gather_comm);
    if (sub_rank == 0)
    {
        MPI_Gather(c, sub_size * sub_size, MPI_INT, buf, sub_size * sub_size, MPI_INT, 0, gather_comm);
    }

    double endGather = MPI_Wtime();

    if (rank == 0)
    {
        int counter = 0;
        for (int l = 0; l < MATRIX_SIZE / sub_size; l++)
        {
            for (int m = 0; m < sub_size; m++)
            {
                for (int n = 0; n < MATRIX_SIZE / sub_size; n++)
                {
                    for (int o = 0; o < sub_size; o++)
                    {
                        matrix_c[counter++] = buf[(l * MATRIX_SIZE * sub_size) + (m * sub_size) + (n * sub_size * sub_size) + o];
                    }
                }
            }
        }
        // printf("Matrix C: \n");
        // printMatrix(matrix_c, MATRIX_SIZE);
        // printf("claculated \n");

        free(matrix_a);
        free(matrix_b);
        free(matrix_c);
        free(temp_a);
        free(temp_b);
    }

    double endTime = MPI_Wtime();

    if (rank == 0)
    {
        printf("Matrix size: %d, Proc mesh size: %d \n", MATRIX_SIZE, PROC);
        printf("Total time: %f \n", endTime - startTime);
        printf("Total without arrayFill: %f \n", (endTime - startTime) - (endArrFill - startArrFill));
        printf("Scatter phase: %f \n", endScatter - startScatter);
        printf("Bcast phase: %f \n", endBcast - startBcast);
        printf("Multiply phase: %f \n", endMatrixMultipy - endBcast);
        printf("Reduce phase: %f \n", endReduce - endMatrixMultipy);
        printf("Gather phase: %f \n", endGather - endReduce);
    }

    free(sub_a);
    free(sub_b);
    free(sub_c);
    free(c);
    free(buf);
    free(ranks_a);
    free(ranks_b);

    MPI_Finalize();

    return 0;
}