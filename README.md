# Dependecies
1. mpicc will be used for compiling. make sure 'mpicc' is executable from the PATH

# Running the program
```bash
# For making the serial executable
make serial

# For making the OpenMP executable
make openmp

# For making the MPI executable
make mpi

# For cleaning all executables
make clean
```


# Folder Structure

```bash
.
├── files
│   ├── 10.txt
│   ├── 11.txt
│   ├── 12.txt
│   ├── 13.txt
│   ├── 14.txt
│   ├── 15.txt
│   ├── 1.txt
│   ├── 2.txt
│   ├── 3.txt
│   ├── 4.txt
│   ├── 5.txt
│   ├── 6.txt
│   ├── 7.txt
│   ├── 8.txt
│   └── 9.txt
├── Makefile
├── mpi_openmp.c
├── mpi_parallel.c
├── mpi_par.c
├── omp_par_rm.c
├── openmp.c
├── openmp_one_q.c
├── output
│   ├── parallel
│   │   ├── 0.dummy
│   ├── serial
│   │   ├── 0.dummy
│   └── serial_results.txt
├── parallel.c
├── parallel_read_map.c
├── parallel_read_map_new.c
├── parallel_RMcomb.c
├── pout
│   ├── 0.dummy
├── README.md
├── scripts
│   ├── get_csv_results.sh
│   ├── run_all.sh
│   ├── submit_mpi_openmp.sub
│   ├── submit_mpi_parallel.sub
│   ├── submit_openmp.sub
│   ├── submit_parallel_read_map.sub
│   ├── submit_parallel.sub
│   └── submit_serial.sub
├── serial.c
├── submit_mpi_openmp.sub
└── util
    ├── file_copy.sh
    ├── hashTable.h
    ├── queue.h
    └── util.h
```

## .
Makefile - refer to the Makefile for commands.
serial.c - serial program src file which is used as the baseline.
parallel.c - OpenMP program where each thread runs Read Map sequentially. This progra does not use a locking mechanism to access data in the queue as Read and Map are run sequentially by each thread.
parallel_read_map.c - OpenMP program where some threads run Read functionality while some threads run Map functionality. This program is based on a locking mechanism for queue data access.
mpi_parallel.c - pure MPI program w/o OpenMP src file.
mpi_openmp.c - MPI w/+ OpenMP src file.

## files
This directory contains input files. Use -f xx for execution where xx denotes the multiplier to multiply the number of files when reading.

## util
This directory contains C language based implementation of queue data structure, hash table data structure, and some utility common functionalities used by differenct source files.

## scripts
This directory contains sbatch submit scripts for each map-reduce implementation.

## output
This directory is used for writing the final word counts of the serial program and OpenMP programs.

## pout
This directory is used for writing the final word counts of the MPI programs.