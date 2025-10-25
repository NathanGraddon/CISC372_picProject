#Config
CC       := gcc
CFLAGS   := -O2 -g -Wall -Wextra -std=gnu99
LDFLAGS  := -lm
#Prefer -pthread; if Darwin insists on -lthread, run:
#    make THREADLIB=-lthread image_pthreads
THREADLIB ?= -pthread

#Default
.PHONY: all
all: image image_pthreads image_omp

#Sequential (starter)
image: image.c image.h stb_image.h stb_image_write.h
	$(CC) $(CFLAGS) -o $@ image.c $(LDFLAGS)

#Pthreads version
image_pthreads: image_pthreads.c image.h stb_image.h stb_image_write.h
	$(CC) $(CFLAGS) $(THREADLIB) -o $@ image_pthreads.c $(LDFLAGS)

#OpenMP version
image_omp: image_omp.c image.h stb_image.h stb_image_write.h
	$(CC) $(CFLAGS) -fopenmp -o $@ image_omp.c $(LDFLAGS)

#Helpers
.PHONY: clean run-seq run-pth run-omp
clean:
	rm -f image image_pthreads image_omp output.png

#Quick local smoke tests 
run-seq: image
	./image pic1.jpg edge

run-pth: image_pthreads
	./image_pthreads pic1.jpg edge 4

run-omp: image_omp
	OMP_NUM_THREADS=4 ./image_omp pic1.jpg edge
