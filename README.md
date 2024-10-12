# Chip-8 Emulator

Chip-8 emulator built for educational purposes, this project is about understanding low-level system architecture, instruction handling, and graphis rendering for the Chip-8 system.

## Features

- Emulation of all Chip-8 opcodes plus some from SCHIP.
- 64x32 monochrome display rendering with different simple color palettes.
- Hi-res mode: 128x64 display support (WIP)
- Support loading Chip-8 ROMs.
- Basic sound output (beeping).

## Usage

```
Usage: chip8_emulator [ROM file] [Selected theme]

  -ROM File: path to the ROM
  -Selected theme: number between 0 and 13 (more can be added manually)
```

### Prerequisited

To compile the emulator you need to have in your system:

- A C/C++ compiler (e.g., 'GCC')
- [SDL2](https://www.libsdl.org/) (Simple DirectMedia Layer) for rendering graphics and handling input.

### Installation

Clone the repository by ```git clone https://github.com/SnLynen/chip8.git```

Run Make or via the CLI run ```gcc -g -o chip8_emulator chip8.c -lSDL2 -lm```

## License

[GNU](https://choosealicense.com/licenses/gpl-3.0/)
