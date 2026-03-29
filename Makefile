CC = mpicc
CFLAGS = -Wall -O2 -fopenmp
TARGET = mandelbrot
CONFIG = config.txt
OUTPUT = mandelbrot.ppm

all: $(TARGET)

$(TARGET): mandelbrot.c
	$(CC) $(CFLAGS) -o $(TARGET) mandelbrot.c -lm

run: $(TARGET) $(CONFIG)
	mpiexec -n 4 ./$(TARGET) $(CONFIG)

clean:
	rm -f $(TARGET) $(OUTPUT)

.PHONY: all run clean