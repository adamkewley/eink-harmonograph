# eink-harmonograph

Arduino project for rendering a
[harmonograph](https://en.wikipedia.org/wiki/Harmonograph) image using
an eink display.

Renders a random harmonograph image as frequently as possible, which
is about once per minute with the settings in this repo.


## Hardware

- Arduino Uno

- Waveshare 4.2 inch E-Ink Display Module ([amazon](https://www.amazon.co.uk/Waveshare-Resolution-Electronic-Interface-Raspberry/dp/B0751J99PS/ref=sr_1_7?s=computers&ie=UTF8&qid=1546344642&sr=1-7&keywords=waveshare+4.2))

- ~8 jumper cables


### Hardware Assembly

- Wire the screen up to the Arduino according to standard manufacturer
  guide (see
  [here](https://www.waveshare.com/wiki/4.2inch_e-Paper_Module))
  
- Connect Arduino to computer

- Build + deploy to the arduino (see below)

- Put it in a nice case

- Done: the device should flash harmonograph images to the screen


## Software Build & Deploy

- Install `third_party/` as an Arduino library. I modified the demo code from
  [here](https://www.waveshare.com/wiki/4.2inch_e-Paper_Module) to fix
  some bugs I encountered (old version of Arduino etc.). The code in
  `third_party/` includes those modifications


### Arduino IDE

This was developed in the standard Arduino IDE (v1.8.8).

TODO


### Arduino CLI

This project includes `Makefile` goals for installing + setting up
[arduino-cli](https://github.com/arduino/arduino-cli). To use those:

```
# install + setup arduino-cli
make setup-arduino-cli 

# compile harmonograph binaries
make compile

# deploy to typical arduino device path
make deploy
```

This can be customized with environment variables (see the
`Makefile`):

```
ARDUINO_CLI="arduino-cli" ARDUINO_DEV=/dev/ttyACM2 make compile deploy
```


## Simulating

Project includes an SDL2 implementation of the renderer. Requires
`libsdl2-dev` to be installed.

```bash
make simulator-run
```

**Note**: The simulator isn't efficient: it's built with a rendering
API that's similar to what the Waveshare eInk display needs (so that
code can be shared between the Arduino and simulator).

## Limitations

- Algorithm is unoptimized: takes ~1 min of constant computation to
  render an image
  
- Because of the above, cannot run the device on a battery (too much
  drain to run it for multiple days)

