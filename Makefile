# Usage:
# make        # compile all binary
# make clean  # remove ALL binaries and objects

CC = gcc
CFLAGS = 
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
	$(CC) $(INCLUDES) -o $@ $< $(LFLAGS) $(LIBS)

.c.o:
	$(CC) $(INCLUDES) -c $< -o $@ $(LIBS)

clean:
	@echo "removing executables.."
	$(RM) *.o *~ $(EXECS)
