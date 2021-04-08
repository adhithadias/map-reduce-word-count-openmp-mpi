cmake_minimum_required(VERSION 2.8.12)

set(PROJECT_NAME map_reduce)
project(${PROJECT_NAME} C CXX)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp -std=c11")

add_executable(serial serial.c)
add_executable(parallel parallel.c)
add_executable(parallel_read_map parallel_read_map.c)
add_executable(mpi_parallel mpi_parallel.c)
