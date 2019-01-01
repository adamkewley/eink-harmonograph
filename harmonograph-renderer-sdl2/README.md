# harmonograph-renderer-sdl2

An SDL2 implementation of a harmonograph

This is an SDL2 implementation of the harmonograph renderer that I
threw together so that I could debug the code locally on my laptop
rather than needing to upload+test the code on the target Arduino. 

The code is here for archiving purposes. It's not a clean
implementation and many of the oddities in it are because it was
refactored to be as close as possible to what the target hardware
needed (e.g. a bitbuffer for pixels, segmented rendering, 2K memory
limit).


# Requirements

- `make`
- `g++`
- `libsdl2-dev`


# Building

```bash
make all
```


# Running

```
make run
```
