#!/usr/bin/env make

export PATH := "${PATH}:target"
ARDUINO_DEV ?= /dev/ttyACM0
ARDUINO_FQBN ?= arduino:avr:uno

target:
	mkdir -p $@

target/arduino-cli: | target
	cd target && wget https://github.com/arduino/arduino-cli/releases/download/0.4.0/arduino-cli_0.4.0_Linux_64bit.tar.gz
	cd target && tar xvf arduino-cli_0.4.0_Linux_64bit.tar.gz


.PHONY: get-arduino-cli setup-arduino-cli compile deploy

get-arduino-cli: target/arduino-cli

setup-arduino-cli: get-arduino-cli
	arduino-cli
	arduino-cli core update-index
	arduino-cli core install arduino:avr

compile:
	arduino-cli compile --fqbn ${ARDUINO_FQBN} harmonograph-renderer-arduino

deploy: compile
	arduino-cli upload -p "${ARDUINO_DEV}" --fqbn ${ARDUINO_FQBN}  harmonograph-renderer-arduino
