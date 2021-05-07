#! /bin/bash
rm -rf slurm* # Cleanup old outputs

# For MPI
sbatch submit_mpi_parallel.sub # Run with 16 processes
# sbatch .sub # Run with processes
# sbatch .sub # Run with processes

sbatch submit_parallel_read_map.sub # Run with 16 processes
sbatch submit_parallel.sub
sbatch submit_serial.sub