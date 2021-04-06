# Usage:
# make        # compile all binary
# make clean  # remove ALL binaries and objects

CC = mpicc
CFLAGS = -std=c11 -std=gnu99 
# INCLUDES = -I../include
# LFLAGS = -L../lib
LIBS = -lm -fopenmp

SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

# SRCS = $(wildcard $(SRCDIR)/*.c)
# OBJS = $(SRCS:.c=.o)
# EXECS = $(SRCS:%.c=%)

SOURCES  := $(wildcard $(SRCDIR)/*.c)
EXECS    := $(SOURCES:$(SRCDIR)/%.c=%)
INCLUDES := $(wildcard $(SRCDIR)/util/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
BINS     := $(SOURCES:$(SRCDIR)/%.c=$(BINDIR)/%)
rm       = rm -f

.PHONY: all clean default

all: ${EXECS}
	@echo  sources compiled

${EXECS}:  %: %.o
	@echo "creating the executable.."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

# .c.o:
# 	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ $(LIBS)

# serial:
# 	mpicc -std=c11 -std=gnu99 -fopenmp -o serial serial.c -fopenmp

# openmp:
# 	mpicc -std=c11 -std=gnu99 -fopenmp -o parallel parallel.c

# omp-par-read-map:
# 	mpicc  -std=c11 -std=gnu99 -fopenmp -o parallel_read_map parallel_read_map.c 

# mpi:
# 	mpicc -std=c11 -std=gnu99  -fopenmp -o mpi_parallel mpi_parallel.c

clean:
	@echo "removing executables.."
	@$(RM) $(OBJECTS)
