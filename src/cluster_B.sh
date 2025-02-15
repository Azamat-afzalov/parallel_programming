#!/usr/bin/env bash
#SBATCH --partition=all
#SBATCH --ntasks=361
#SBATCH --job-name=my_job_name
#SBATCH --output=test_result_%A_%a.log
#SBATCH --time=00:10:00
#SBATCH --array=1-10

SOURCE_FILE="mpi_taskB.c"

# Define an array of configurations
CONFIGS=(
    "800 64"
    "800 361"
    "2000 64"
    "2000 361"
    "4000 64"
    "4000 361"
    "6000 64"
    "6000 361"
    "8000 64"
    "8000 361"
)

# Extract MATRIX_SIZE and NUM_PROCESSES from the array using SLURM_ARRAY_TASK_ID
CONFIG="${CONFIGS[$SLURM_ARRAY_TASK_ID - 1]}"
read MATRIX_SIZE NUM_PROCESSES <<< "$CONFIG"

EXECUTABLE="mpi_taskB_${MATRIX_SIZE}_${NUM_PROCESSES}"

echo "Running (MATRIX_SIZE=$MATRIX_SIZE - np=$NUM_PROCESSES)"
mpicc -o $EXECUTABLE -D MATRIX_SIZE=$MATRIX_SIZE -D PRINT=false $SOURCE_FILE
mpirun -np $NUM_PROCESSES ./$EXECUTABLE
