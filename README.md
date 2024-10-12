# Chip-8 Emulator

This is a **Chip-8 emulator** built for educational purposes. The goal of this project is to gain a deeper understanding of low-level system architecture, instruction handling, and graphics rendering. Chip-8 is a simple, interpreted programming language used to run games on early computing systems.

## Features

- Full emulation of all Chip-8 opcodes, plus some SCHIP extensions.
- 64x32 monochrome display rendering with customizable color palettes.
- **Hi-res mode**: 128x64 display support (**Work In Progress**).
- Support for loading and running Chip-8 ROMs.
- Basic sound output (beeping on specific instructions).

  ![alt text](https://github.com/SnLynen/chip8/src/blob/main/preview.jpg?raw=true)

## Usage

Usage: chip8_emulator [ROM file] [Selected theme]

    - ROM File: path to the ROM file to load.
    - Selected theme: a number between 0 and 13 to select one of the pre-configured themes (more themes can be added manually).

### Example:

```bash
./chip8_emulator roms/PONG.ch8 2
```

## Prerequisites

To compile the emulator you need to have in your system:

- A C/C++ compiler (e.g., 'GCC')
- [SDL2](https://www.libsdl.org/) (Simple DirectMedia Layer) for rendering graphics and handling input.

## Installation

1. Clone the repository:

```bash
git clone https://github.com/SnLynen/chip8.git
cd chip8
```

2. Install SDL2 (if not installed):

  - On Linux (Debian-based): ```sudo apt-get install libsdl2-dev```
  - On macOS (Using Homebrew): ```brew install sdl2```
  - On Windows: Download SDL2 from the [official SDL2 website](https://www.libsdl.org/) and set up according to your environment.

3. Build the emulator :

```bash
gcc -g -o chip8_emulator chip8.c -lSDL2 -lm
```

4. Run the emulator:

```bash
./chip8_emulator roms/PONG.ch8 0
```

## License

This project is licensed under the [GNU General Public License v3.0](https://choosealicense.com/licenses/gpl-3.0/). You are free to use, modify, and distribute this software under the terms of the license.
