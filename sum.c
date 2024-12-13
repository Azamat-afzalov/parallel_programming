#include<stdio.h>
#include<stdbool.h>
#include<pthread.h>
#include<stdlib.h>
#include<omp.h>

#define NUM_THREADS 10
#define ITERATIONS 100000000 // 100 million

long long int sum = 0;
pthread_mutex_t lock;

void *calculate_sum(void *arg) {
    int thread_id = *(int*)arg;
    long int partial_sum = 0;

    int start = thread_id * (ITERATIONS / NUM_THREADS);
    int end = start + (ITERATIONS / NUM_THREADS);


    for (int i = start + 1; i <= end; i++) {
        partial_sum += i;
    }

    // lock the mutex to change global sum;
    pthread_mutex_lock(&lock);
    sum += partial_sum;
    // unlock the mutext after changing global sum;
    pthread_mutex_unlock(&lock);

    pthread_exit(NULL);
}

long long calculate_sum_sections() {
    long long int sum = 0;
    long long int section_1 = 0;
    long long int section_2 = 0;


    #pragma omp parallel 
    {
        #pragma omp sections 
        {
            // calculate 1 to 50 million 
            #pragma omp section
            {
                for (int i = 1; i <= ITERATIONS / 2; i++) {
                    section_1 += i;
                }
            }

            // calculate 50,000,001 to 100 million
            #pragma omp section 
            {
                for (int i = (ITERATIONS / 2) + 1; i <= ITERATIONS; i++) {
                    section_2 += i;
                }
            }
        }
        #pragma omp barrier
        sum = section_1 + section_2;
    }
    
    return sum;
}

long long calculate_sum_tasks() {
    long long int sum = 0;

    #pragma omp parallel 
    {
        #pragma omp single 
        {
            #pragma omp task shared(sum)
            {
                long long local_sum = 0;
                for (int i = 1; i <= ITERATIONS / 2; i++) {
                    local_sum += i;
                }

                #pragma omp atomic
                sum += local_sum;
            }

            #pragma omp task shared(sum)
            {
                long long local_sum = 0;
                for (int i = (ITERATIONS/ 2) + 1; i <= ITERATIONS; i++) {
                    local_sum += i;
                }

                #pragma omp atomic
                sum += local_sum;
            }

            #pragma omp taskwait
        }
    }

    return sum;
}


int main() {
    double wt1, wt2;
    omp_set_num_threads(NUM_THREADS);
    // Calculate sum with pthreads
    wt1 = omp_get_wtime();
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    pthread_mutex_init(&lock, NULL);
    
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;

        if (pthread_create(&threads[i], NULL, calculate_sum, &thread_ids[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&lock);

    wt2 = omp_get_wtime();
    printf("Pthreads = %12.4g sec \n", wt2 - wt1);

    // Calculate sum with OpenMP
    wt1 = omp_get_wtime();
    long long int omp_sum = 0;
    #pragma parallel for reduction(+:sum)
    for(int i = 1; i <= ITERATIONS; i++) {
        omp_sum += i;
    }

    wt2 = omp_get_wtime();
    printf("OpenMP = %12.4g sec \n", wt2 - wt1);

    // Calculate sum with sections
    wt1 = omp_get_wtime();
    long long int sections = calculate_sum_sections();
    
    wt2 = omp_get_wtime();
    printf("Sections = %12.4g sec \n", wt2 - wt1);

    // Calculate sum with tasks
    wt1 = omp_get_wtime();
    long long int tasks = calculate_sum_tasks();

    wt2 = omp_get_wtime();
    printf("Tasks = %12.4g sec \n", wt2 - wt1);


    return 0;
}