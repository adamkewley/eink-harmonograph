#!/usr/bin/make

target/:
	mkdir -p $@

.PHONY: all run

all: | target/
	g++ -std=c++11 -O3 main.cpp -lSDL2 -lm -o target/harmonograph-renderer-sdl2

run: all
	target/harmonograph-renderer-sdl2
