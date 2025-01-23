#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <locale.h>

// #define SIZE 10000000 // 10 million
#define THRESHOLD 20000

void q_sort(int* arr, int start_index, int end_index) {
    if (start_index >= end_index) {
        return; // Base case: array has one or zero elements
    }

    int pivot = end_index;

    int replace_index = start_index;

    for (int i = start_index; i < end_index; i++) {
        if (arr[i] < arr[pivot]) {
            int tmp = arr[i];
            arr[i] = arr[replace_index];
            arr[replace_index] = tmp;  
            replace_index++;
        }
    }

    int tmp = arr[pivot];
    arr[pivot] = arr[replace_index];
    arr[replace_index] = tmp;  
    
    if (end_index - start_index > THRESHOLD) {
        #pragma omp task
        q_sort(arr, start_index, replace_index - 1);

        #pragma omp task
        q_sort(arr, replace_index + 1, end_index);

        // #pragma omp taskwait
    } else {

        q_sort(arr, start_index, replace_index - 1);
        q_sort(arr, replace_index + 1, end_index);
    }
}


int main(int argc, char *argv[]) {

    // To measure time
    double wt1, wt2;

    int SIZE = atoi(argv[1]);


    int *nums = (int*)malloc(SIZE * sizeof(int));
    
    srand(time(0));

    for (int i = 0; i < SIZE; i++) {
        nums[i] = rand() % 2001 - 1000; // Random numbers range [-1000, 1000]
    }
    
    wt1 = omp_get_wtime();


    int last_index = SIZE - 1;
    #pragma omp parallel 
    {
        #pragma omp single 
        {
            q_sort(nums, 0, last_index);
        }
    }
    
    free(nums);
    
    wt2 = omp_get_wtime();

    setlocale(LC_NUMERIC, "");
    // printf( "Parallel excecution with Size: %'d \n", SIZE);
    printf( "%'d : \t\t %12.4g sec\n", SIZE, wt2-wt1 );
    // printf("================================================= \n");
    return 0;
}
