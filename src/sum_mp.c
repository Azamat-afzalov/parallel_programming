#include<omp.h>
#include<stdio.h>

#define NUM_THREADS 10
#define END 100000000 // 100 milliom

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
                for (int i = 1; i <= END / 2; i++) {
                    section_1 += i;
                }
            }

            // calculate 50,000,001 to 100 million
            #pragma omp section 
            {
                for (int i = (END / 2) + 1; i <= END; i++) {
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
                for (int i = 1; i <= END / 2; i++) {
                    local_sum += i;
                }

                #pragma omp atomic
                sum += local_sum;
            }

            #pragma omp task shared(sum)
            {
                long long local_sum = 0;
                for (int i = (END/ 2) + 1; i <= END; i++) {
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
    omp_set_num_threads(NUM_THREADS);
    long long int sum = 0;

    #pragma parallel for reduction(+:sum)
    for(int i = 1; i <= END; i++) {
        sum += i;
    }
    
    // Calculate actual sum with Gaussian formula
    long long int actual = (long long)(END) * (END + 1) / 2 ;
    
    // Calculate sum with sections
    long long int sections = calculate_sum_sections();

    // Calculate sum with tasks
    long long int tasks = calculate_sum_tasks();

    printf("Calculated Sum = %lld \n", sum);
    printf("Calculated Sum (Sections) = %lld \n", sections);
    printf("Calculated Sum (Tasks) = %lld \n", tasks);
    printf("Actual = %lld \n", actual);
    
    return 0;
}