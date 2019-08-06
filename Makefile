#!/usr/bin/env make

.PHONY: validate compile deploy

validate:
	wget https://github.com/arduino/arduino-cli/releases/download/0.4.0/arduino-cli_0.4.0_Linux_64bit.tar.gz
	tar xvf arduino-cli_0.4.0_Linux_64bit.tar.gz
	./arduino-cli
	./arduino-cli core update-index
	./arduino-cli core install arduino:avr

compile: validate
	./arduino-cli compile --fqbn arduino:avr:uno harmonograph-renderer-arduino

deploy: compile
	./arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:uno  harmonograph-renderer-arduino
