#ifndef CONFIG_H
#define CONFIG_H

#include <SDL2/SDL.h>
#include "colorp.h"

#define LOW_RES_WIDTH 64
#define LOW_RES_HEIGHT 32
#define HIGH_RES_WIDTH 128
#define HIGH_RES_HEIGHT 64

struct Chip8 {
    unsigned short opcode;
    unsigned char memory[4096];
    unsigned char V[16];
    unsigned short I;
    unsigned short pc;
    unsigned char gfx[HIGH_RES_WIDTH * HIGH_RES_HEIGHT];
    unsigned char delay_timer;
    unsigned char sound_timer;
    unsigned short stack[16];
    unsigned short sp;
    unsigned char key[16];
    uint16_t KP; // Previous key states
    uint16_t KC; // Current key states
    uint16_t IK; // Input keys
    uint8_t keyReg; // Register index for FX0A instruction
    uint8_t carry;
    bool drawFlag;
    bool compatMode;
    bool highRes;
    bool legacyMode;
    bool xoChipMode;
    unsigned char fontSet[80];
    bool waitingForKey;
    bool keyReleased;
    double IPC;
    double cD;
    uint8_t plane;

    SDL_AudioSpec beepSpec;
    SDL_AudioDeviceID beepDevice;
    struct Theme theme; // Use Theme struct for colors
} chip8;

#endif // CONFIG_H