#!/usr/bin/env bash
#SBATCH --partition=all
#SBATCH --ntasks=64
#SBATCH --job-name=my_job_name                                                                                          
#SBATCH --output=test_result.log                                                                                        
#SBATCH --time=00:10:00                                                                                                                                                                                                                                                                                                                                                 
SOURCE_FILE="mpi_taskB.c"                                                                                                                                                                                                                          
EXECUTABLE="mpi_task_b"

echo "(800 - 64): \n"
mpicc -o $EXECUTABLE -D MATRIX_SIZE=800 -D PRINT=false $SOURCE_FILE -lm                                                                                                                                                                          
mpirun -np 64 ./$EXECUTABLE
echo "\n"
# echo "(800 - 361): \n"
# mpicc -o $EXECUTABLE -D MATRIX_SIZE=800 -D PRINT=false $SOURCE_FILE                                                                                                                                                                                 
# mpirun -np 361 ./$EXECUTABLE
# echo "\n"
# echo "(2000 - 64): \n"
# mpicc -o $EXECUTABLE -D MATRIX_SIZE=2000 -D PRINT=false $SOURCE_FILE                                                                                                                                                                                 
# mpirun -np 64 ./$EXECUTABLE
# echo "\n"
# echo "(2000 - 361): \n"
# mpicc -o $EXECUTABLE -D MATRIX_SIZE=2000 -D PRINT=false $SOURCE_FILE                                                                                                                                                                                 
# mpirun -np 361 ./$EXECUTABLE
# echo "\n"
# echo "(4000 - 64): \n"
# mpicc -o $EXECUTABLE -D MATRIX_SIZE=4000 -D PRINT=false $SOURCE_FILE                                                                                                                                                                                 
# mpirun -np 64 ./$EXECUTABLE
# echo "\n"
# echo "(4000 - 361): \n"
# mpicc -o $EXECUTABLE -D MATRIX_SIZE=4000 -D PRINT=false $SOURCE_FILE                                                                                                                                                                                 
# mpirun -np 361 ./$EXECUTABLE
# echo "\n"
# echo "(6000 - 64): \n"
# mpicc -o $EXECUTABLE -D MATRIX_SIZE=6000 -D PRINT=false $SOURCE_FILE                                                                                                                                                                                 
# mpirun -np 64 ./$EXECUTABLE
# echo "\n"
# echo "(6000 - 361): \n"
# mpicc -o $EXECUTABLE -D MATRIX_SIZE=6000 -D PRINT=false $SOURCE_FILE                                                                                                                                                                                 
# mpirun -np 361 ./$EXECUTABLE
# echo "\n"

# echo "(8000 - 64): \n"
# mpicc -o $EXECUTABLE -D MATRIX_SIZE=8000 -D PRINT=false $SOURCE_FILE                                                                                                                                                                                 
# mpirun -np 64 ./$EXECUTABLE
# echo "\n"
# echo "(8000 - 361): \n"
# mpicc -o $EXECUTABLE -D MATRIX_SIZE=8000 -D PRINT=false $SOURCE_FILE                                                                                                                                                                                 
# mpirun -np 361 ./$EXECUTABLE
# echo "\n"


