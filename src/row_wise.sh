#!/bin/bash

# Compile the MPI program
mpicc -o row_wise row_wise_mult.c
if [ $? -ne 0 ]; then
    echo "Compilation failed."
    exit 1
fi

# Run the MPI program with 4 processes
mpirun --map-by :OVERSUBSCRIBE -np 16 ./row_wise
