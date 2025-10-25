#config
CC      := gcc
CFLAGS  := -O2 -g -Wall -Wextra
#if grader/toolchain requires -lthread, build with:
#   make THREADLIB=-lthread
THREADLIB ?= -pthread

#default target
.PHONY: all
all: image image_pthreads image_omp

#sequential(original)
#Builds the starter single-threaded program from image.c
image: image.c image.h stb_image.h stb_image_write.h
	$(CC) $(CFLAGS) -o $@ image.c -lm

#pthreads version
image_pthreads: image_pthreads.c image.h stb_image.h stb_image_write.h
	$(CC) $(CFLAGS) $(THREADLIB) -o $@ image_pthreads.c -lm

#OpenMP version
image_omp: image_omp.c image.h stb_image.h stb_image_write.h
	$(CC) $(CFLAGS) -fopenmp -o $@ image_omp.c -lm

#utilities
.PHONY: clean run-seq run-pth run-omp
clean:
	rm -f image image_pthreads image_omp output.png

#quick helpers for local testing
run-seq: image
	./image pic1.jpg edge

run-pth: image_pthreads
	./image_pthreads pic1.jpg edge 4

run-omp: image_omp
	OMP_NUM_THREADS=4 ./image_omp pic1.jpg edge
