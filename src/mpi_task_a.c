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

int main() {

    MPI_Init(NULL, NULL);

    int rank, size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int matrix_size = (int)sqrt(size);

    int num_of_dimensions = matrix_size;                    
    int dims[2] = {0, 0}; // This will be {2,2} with 4 processes
    int periods[2] = {1, 1};  // Lets do it periodic 
    int reorder = 0; 
    int coords[2]; 
    int comm_rank;

    MPI_Comm cart_comm; // Cartesian topology


    MPI_Dims_create(size, num_of_dimensions, dims);
    MPI_Cart_create(MPI_COMM_WORLD, num_of_dimensions, dims, periods, 1, &cart_comm);
    MPI_Cart_coords(cart_comm, rank, num_of_dimensions, coords);
    MPI_Cart_rank(cart_comm, coords, &comm_rank);


    srand(time(NULL) + rank);

    int i = rank / matrix_size;
    int j = rank % matrix_size;

    int received_a;
    int received_b;
    
    int A[size];
    int B[size];
    int C[size];

    // printf("COMM RANK %d, %d \n", comm_rank, rank);

   

    if (rank == 0) {
        
        for (int i = 0; i < size; i++) {
            A[i] = rand() % 50; 
            B[i] = rand() % 50;
            // MPI_Send(&A[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD);  // tag 0 -> A[i][j]
            // MPI_Send(&B[i], 1, MPI_INT, i, 1, MPI_COMM_WORLD);  // tag 1 -> B[i][j]
        }
        

        print_matrix(A, size, matrix_size, "A");
        print_matrix(B, size, matrix_size, "B");


        // We will do this number of wraparounds.
        // If matrix size is 3x3 we need to do 3 wraparounds

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



    // MPI_Barrier(MPI_COMM_WORLD);

    MPI_Scatter(A, 1, MPI_INT, &received_a, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(B, 1, MPI_INT, &received_b, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // printf("Rank %d received A[%d][%d] received %d \n",rank, i, j, received_a);
    // printf("Rank %d received B[%d][%d] received %d \n",rank, i, j, received_b);
    // printf("Rank: %d, Cords: (%d, %d)", rank, coords[0], coords[1]);

    MPI_Comm row_comm;
    // int x_dims[2] = {1, 0};

    MPI_Comm_split(cart_comm, coords[0], rank, &row_comm);

    int diagonal = -1;
    
    if (coords[0] == coords[1]) {
        // printf("Diagonal element: A[%d][%d] = %d\n", coords[0], coords[1], received_a);
        diagonal = received_a;
    }
    
    MPI_Bcast(&diagonal, 1, MPI_INT, coords[0], row_comm);


    printf("Received Diagonal=%d, for row B=%d \n", diagonal, received_b);
   

    MPI_Comm_free(&row_comm);
    MPI_Comm_free(&cart_comm);
    MPI_Finalize();
    return 0;
}
