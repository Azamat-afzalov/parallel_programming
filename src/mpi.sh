#!/bin/bash

# Compile the MPI program
mpicc -o task_a mpi_task_a.c
if [ $? -ne 0 ]; then
    echo "Compilation failed."
    exit 1
fi

# Run the MPI program with 4 processes
mpiexec -n 4 ./task_a
