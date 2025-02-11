#!/bin/bash

# Compile the MPI program
mpicc -o task_B mpi_taskB.c
if [ $? -ne 0 ]; then
    echo "Compilation failed."
    exit 1
fi

# Run the MPI program with 4 processes
mpirun --map-by :OVERSUBSCRIBE -np 9 ./task_B
