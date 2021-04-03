# Usage:
# make        # compile all binary
# make clean  # remove ALL binaries and objects

CC = mpicc
CFLAGS = -std=c11
INCLUDES = -I../include
LFLAGS = -L../lib
LIBS = -lm -fopenmp

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
EXECS = $(SRCS:%.c=%)

.PHONY: all clean

all: ${EXECS}
	@echo  sources compiled

${EXECS}:  %: %.o
	@echo "creating the executable.."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ $(LIBS)

serial:
	mpicc -std=c11 -o serial serial.c -fopenmp

openmp:
	mpicc -std=c11 -o parallel parallel.c -fopenmp

mpi:
	mpicc -std=c11 -o mpi_parallel mpi_parallel.c -fopenmp

clean:
	@echo "removing executables.."
	$(RM) *.o *~ $(EXECS)
