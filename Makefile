#!/usr/bin/env make

ARDUINO_DEV ?= /dev/ttyACM0
ARDUINO_FQBN ?= arduino:avr:uno
ARDUINO_CLI ?= dependencies/arduino-cli

dependencies:
	mkdir -p $@

dependencies/arduino-cli: | dependencies
	cd dependencies && wget https://github.com/arduino/arduino-cli/releases/download/0.4.0/arduino-cli_0.4.0_Linux_64bit.tar.gz
	cd dependencies && tar xvf arduino-cli_0.4.0_Linux_64bit.tar.gz

target:
	mkdir -p $@

target/eink-harmonograph-simulator: eink_harmonograph_sdl2.cpp | target
	c++ -std=c++11 -O2 -g eink_harmonograph_sdl2.cpp -lSDL2 -lm -o $@

target/eink-harmonograph.elf: eink-harmonograph/eink-harmonograph.ino | target
	${ARDUINO_CLI} compile --fqbn ${ARDUINO_FQBN} eink-harmonograph


.PHONY: deps-get-arduino-cli deps-setup-arduino-cli compile deploy simulator-build simulator-run clean


# Arduino

deps-get-arduino-cli: dependencies/arduino-cli
deps-setup-arduino-cli: deps-get-arduino-cli
	${ARDUINO_CLI}
	${ARDUINO_CLI} core update-index
	${ARDUINO_CLI} core install arduino:avr

compile: target/eink-harmonograph.elf

deploy: compile
	${ARDUINO_CLI} upload -p "${ARDUINO_DEV}" --fqbn ${ARDUINO_FQBN} eink-harmonograph


# simulator

simulator-build: target/eink-harmonograph-simulator

simulator-run: simulator-build
	target/eink-harmonograph-simulator

clean:
	rm -rf target
	rm -f eink-harmonograph/eink-harmonograph.hex
	rm -f eink-harmonograph/eink-harmonograph.elf
