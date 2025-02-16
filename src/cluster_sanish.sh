#!/usr/bin/env bash
#SBATCH --partition=all
#SBATCH --ntasks=361
#SBATCH --job-name=my_job_name
#SBATCH --output=test_result.log
#SBATCH --time=00:10:00

SOURCE_FILE="mpi_sanish_taskB.c"

# Define an array of MATRIX_SIZE values
# MATRIX_SIZES=(800 2000 4000 6000 8000)
# NUM_PROCESSES=(64 361)

# for SIZE in "${MATRIX_SIZES[@]}"; do
#     for NP in "${NUM_PROCESSES[@]}"; do
#         EXECUTABLE="mpi_sanish_taskB_${SIZE}_${NP}"
        
#         echo "Running (MATRIX_SIZE=$SIZE - np=$NP)"
#         mpicc -o $EXECUTABLE -D MATRIX_SIZE=$SIZE $SOURCE_FILE 
#         mpirun -np $NP ./$EXECUTABLE
#         echo "\n"
#     done
# done

EXECUTABLE="mpi_sanish_taskB"
SIZE=8000
NP=361

echo "Running (MATRIX_SIZE=8000 - np=361)"
mpicc -o $EXECUTABLE -D MATRIX_SIZE=$SIZE $SOURCE_FILE 
mpirun -np $NP ./$EXECUTABLE
echo "\n"