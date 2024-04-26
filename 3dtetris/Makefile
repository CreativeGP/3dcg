# CC = gcc
# CFLAGS = $(shell pkg-config --cflags bullet)
# LDFLAGS = 

main:
	g++ main.cpp -O0 $(shell pkg-config --cflags --libs bullet)   -lGLEW -DGLEW_STATIC -lglfw -lglut -lGL -lGLU -lm -lSDL2 -o main -g3
