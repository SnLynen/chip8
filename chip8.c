#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <time.h>
#include "src/config.h"
#include "src/colorp.h"

// User-defined types
#define instructionPerCycle 8;
#define cycleDuration 16;

// Function prototypes
int initSDL2();
void initChip8();
int loadRom(const char *rom);
void drawGfx(SDL_Renderer *renderer);
void inputCycle(SDL_Event event);
void updateInput();
void handleInterrupts(int *SPEED, int *interruptType);
void updateTimers();
void emulateCycle(int *SPEED, int *interruptType);
void initAudio();
void soundBeep();

// Global variables
bool running = true;
SDL_Renderer *renderer = NULL;
int currentTheme = 0; // Default theme

// Main function
int main(int argc, char *argv[]) {

    int SPEED = 8; // Number of instructions per frame
    int interruptType = -1; // -1 means no interrupt

    // Check if SDL2 was initialized
    if(!initSDL2()) EXIT_FAILURE;
    SDL_Window *window = SDL_CreateWindow("CHIP-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 320, SDL_WINDOW_SHOWN);
    // Check if window was created
    if(!window) {
        printf("Window could not be created. %s\n", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    initAudio();

    // Create renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    // Check if renderer was created
    if(!renderer) {
        printf("Renderer could not be created. %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Check if ROM was provided
    if (argc < 2) {
        printf("Usage: %s <ROM>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Check if theme was provided
    if (argc >= 3) {
        currentTheme = atoi(argv[2]);
        if (currentTheme < 0 || currentTheme >= (int)(sizeof(themes) / sizeof(struct Theme))) {
            printf("Invalid theme. Using default theme.\n");
            currentTheme = 0;
        }
    }

    // Initialize Chip8
    initChip8();

    // Load ROM
    if(loadRom(argv[1]) == -1) EXIT_FAILURE;

    SDL_Event event;

    // Main loop
    while (running) {
        uint32_t frameStart = SDL_GetTicks();

        // Decrement timers
        updateTimers();

        // Check for events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            inputCycle(event);
        }

        // Emulate instructions
        for (int i = 0; i < abs(SPEED); ++i) {
            if (SPEED <= 0) break; // Quit early if SPEED is negative
            emulateCycle(&SPEED, &interruptType);
        }

        // Update input values
        updateInput();

        // Handle interrupts
        handleInterrupts(&SPEED, &interruptType);

        // Draw graphics
        if (chip8.drawFlag) drawGfx(renderer);

        // Frame rate control
        uint32_t frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < chip8.cD) {
            SDL_Delay(chip8.cD - frameTime);
        }
    }

    // Close SDL2
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}



// Initialize SDL2
int initSDL2(){
    if(SDL_Init(SDL_INIT_VIDEO)==-1) {
        printf("SDL2 could not be initialized. %s\n", SDL_GetError());
        return 0;
    }
    return 1;
}

// Initialize Chip8
void initChip8() {
    chip8.pc = 0x200;
    chip8.opcode = 0;
    chip8.I = 0;
    chip8.sp = 0;
    chip8.delay_timer = 0;
    chip8.sound_timer = 0;
    memset(chip8.memory, 0, 4096);
    memset(chip8.V, 0, 16);
    memset(chip8.gfx, 0, HIGH_RES_WIDTH * HIGH_RES_HEIGHT);
    memset(chip8.stack, 0, 16 * sizeof(uint16_t)); // Assuming stack is of type uint16_t
    chip8.waitingForKey = false;
    chip8.compatMode = false;
    chip8.highRes = false;
    chip8.legacyMode = false;
    chip8.IPC = instructionPerCycle;
    chip8.cD = cycleDuration;
    chip8.plane = 1;
    chip8.theme = themes[currentTheme];


    struct Chip8 chip8 = {
        .fontSet = {
            0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
            0x20, 0x60, 0x20, 0x20, 0x70, // 1
            0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
            0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
            0x90, 0x90, 0xF0, 0x10, 0x10, // 4
            0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
            0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
            0xF0, 0x10, 0x20, 0x40, 0x40, // 7
            0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
            0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
            0xF0, 0x90, 0xF0, 0x90, 0x90, // A
            0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
            0xF0, 0x80, 0x80, 0x80, 0xF0, // C
            0xE0, 0x90, 0x90, 0x90, 0xE0, // D
            0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
            0xF0, 0x80, 0xF0, 0x80, 0x80  // F
        }
    };

    // Load fontSet
    for(int i = 0; i < 80; ++i) {
        chip8.memory[i] = chip8.fontSet[i];
    }
}

// Load ROM
int loadRom(const char *rom){
    FILE *file = fopen(rom, "rb");
    if(!file) {
        printf("File could not be opened.\n");
        return 0;
    };

    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    // Allocate memory to store the file
    char *buffer = (char*)malloc(sizeof(char)*size);
    if(!buffer) {
        printf("Memory could not be allocated.\n");
        return -1;
    };

    // Copy the file into the buffer
    const size_t result = fread(buffer, 1, size, file);
    if ((size_t)result != (size_t)size) {
        printf("Reading error.\n");
        return -1;
    };

    // Check if ROM fits in memory
    if((4096 - 512) > size) {
        for(int i = 0; i < size; ++i) {
            chip8.memory[i + 512] = buffer[i];
        }
    } else {
        printf("ROM too big for memory.\n");
        return -1;
    }

    // Close file and free buffer
    fclose(file);
    free(buffer);
    return 0;
}

// Input cycle
void inputCycle(const SDL_Event event) {
    switch (event.type){
        // Check if a key was pressed
        case SDL_KEYDOWN:
            if (!chip8.waitingForKey) {
                // Check which key was pressed
                switch (event.key.keysym.sym){
                    case SDLK_1: chip8.key[0x1] = 1; break;
                    case SDLK_2: chip8.key[0x2] = 1; break;
                    case SDLK_3: chip8.key[0x3] = 1; break;
                    case SDLK_4: chip8.key[0xC] = 1; break;
                    case SDLK_q: chip8.key[0x4] = 1; break;
                    case SDLK_w: chip8.key[0x5] = 1; break;
                    case SDLK_e: chip8.key[0x6] = 1; break;
                    case SDLK_r: chip8.key[0xD] = 1; break;
                    case SDLK_a: chip8.key[0x7] = 1; break;
                    case SDLK_s: chip8.key[0x8] = 1; break;
                    case SDLK_d: chip8.key[0x9] = 1; break;
                    case SDLK_f: chip8.key[0xE] = 1; break;
                    case SDLK_z: chip8.key[0xA] = 1; break;
                    case SDLK_x: chip8.key[0x0] = 1; break;
                    case SDLK_c: chip8.key[0xB] = 1; break;
                    case SDLK_v: chip8.key[0xF] = 1; break;
                    default: break;
                }
                chip8.waitingForKey = true;
            }
        break;
        // Check if a key was released
        case SDL_KEYUP:
            // Check which key was released
                switch (event.key.keysym.sym){
                    case SDLK_1: chip8.key[0x1] = 0; break;
                    case SDLK_2: chip8.key[0x2] = 0; break;
                    case SDLK_3: chip8.key[0x3] = 0; break;
                    case SDLK_4: chip8.key[0xC] = 0; break;
                    case SDLK_q: chip8.key[0x4] = 0; break;
                    case SDLK_w: chip8.key[0x5] = 0; break;
                    case SDLK_e: chip8.key[0x6] = 0; break;
                    case SDLK_r: chip8.key[0xD] = 0; break;
                    case SDLK_a: chip8.key[0x7] = 0; break;
                    case SDLK_s: chip8.key[0x8] = 0; break;
                    case SDLK_d: chip8.key[0x9] = 0; break;
                    case SDLK_f: chip8.key[0xE] = 0; break;
                    case SDLK_z: chip8.key[0xA] = 0; break;
                    case SDLK_x: chip8.key[0x0] = 0; break;
                    case SDLK_c: chip8.key[0xB] = 0; break;
                    case SDLK_v: chip8.key[0xF] = 0; break;
                    default: break;
                }
        chip8.waitingForKey = false;
        break;
        default: break;
    }
}

// Emulate one cycle
void emulateCycle(int *SPEED, int *interruptType) {
    // Fetch opcode
        chip8.opcode = chip8.memory[chip8.pc] << 8 | chip8.memory[chip8.pc + 1];
        chip8.pc += 2;
        // Decode opcode
    // Decode opcode
    switch (chip8.opcode & 0xF000) {
        case 0x0000:
            switch (chip8.opcode & 0x00FF) {
                case 0x00E0: // Clear the screen
                    memset(chip8.gfx, 0, 64 * 32);
                    chip8.drawFlag = true;
                    break;
                case 0x00EE: // Return from subroutine
                    chip8.sp--;
                    chip8.pc = chip8.stack[chip8.sp];
                    break;
                case 0x00C0: // 00CN: Scroll display N lines down
                {
                    uint8_t n = chip8.opcode & 0x000F;
                    int width = chip8.highRes ? HIGH_RES_WIDTH : LOW_RES_WIDTH;
                    int height = chip8.highRes ? HIGH_RES_HEIGHT : LOW_RES_HEIGHT;

                    if (n > 0) {
                        for (int plane = 0; plane < 2; ++plane) {
                            if (chip8.plane & (1 << plane)) {
                                for (int y = height - 1; y >= n; --y) {
                                    for (int x = 0; x < width; ++x) {
                                        chip8.gfx[(y * width + x) + (plane * width * height)] = chip8.gfx[((y - n) * width + x) + (plane * width * height)];
                                    }
                                }
                                for (int y = 0; y < n; ++y) {
                                    for (int x = 0; x < width; ++x) {
                                        chip8.gfx[(y * width + x) + (plane * width * height)] = 0;
                                    }
                                }
                            }
                        }
                        chip8.drawFlag = true;
                    }
                }
                break;
                case 0x00D0: // 00DN: Scroll display N lines up
                {
                    uint8_t n = chip8.opcode & 0x000F;
                    int width = chip8.highRes ? HIGH_RES_WIDTH : LOW_RES_WIDTH;
                    int height = chip8.highRes ? HIGH_RES_HEIGHT : LOW_RES_HEIGHT;

                    if (n > 0) {
                        for (int plane = 0; plane < 2; ++plane) {
                            if (chip8.plane & (1 << plane)) {
                                for (int y = 0; y < height - n; ++y) {
                                    for (int x = 0; x < width; ++x) {
                                        chip8.gfx[(y * width + x) + (plane * width * height)] = chip8.gfx[((y + n) * width + x) + (plane * width * height)];
                                    }
                                }
                                for (int y = height - n; y < height; ++y) {
                                    for (int x = 0; x < width; ++x) {
                                        chip8.gfx[(y * width + x) + (plane * width * height)] = 0;
                                    }
                                }
                            }
                        }
                        chip8.drawFlag = true;
                    }
                }
                break;
                case 0x00FB: // 00FB: Scroll display 4 pixels right
                {
                    int width = chip8.highRes ? HIGH_RES_WIDTH : LOW_RES_WIDTH;
                    int height = chip8.highRes ? HIGH_RES_HEIGHT : LOW_RES_HEIGHT;
                    int scrollAmount = chip8.legacyMode && !chip8.highRes ? 2 : 4;

                    for (int plane = 0; plane < 2; ++plane) {
                        if (chip8.plane & (1 << plane)) {
                            for (int y = 0; y < height; ++y) {
                                for (int x = width - 1; x >= scrollAmount; --x) {
                                    chip8.gfx[y * width + x] = chip8.gfx[y * width + (x - scrollAmount)];
                                }
                                for (int x = 0; x < scrollAmount; ++x) {
                                    chip8.gfx[y * width + x] = 0;
                                }
                            }
                        }
                    }
                    chip8.drawFlag = true;
                }
                break;
                case 0x00FC: // 00FC: Scroll display 4 pixels left
                {
                    int width = chip8.highRes ? HIGH_RES_WIDTH : LOW_RES_WIDTH;
                    int height = chip8.highRes ? HIGH_RES_HEIGHT : LOW_RES_HEIGHT;
                    int scrollAmount = chip8.legacyMode && !chip8.highRes ? 2 : 4;

                    for (int plane = 0; plane < 2; ++plane) {
                        if (chip8.plane & (1 << plane)) {
                            for (int y = 0; y < height; ++y) {
                                for (int x = 0; x < width - scrollAmount; ++x) {
                                    chip8.gfx[y * width + x] = chip8.gfx[y * width + (x + scrollAmount)];
                                }
                                for (int x = width - scrollAmount; x < width; ++x) {
                                    chip8.gfx[y * width + x] = 0;
                                }
                            }
                        }
                    }
                    chip8.drawFlag = true;
                }
                break;
                case 0x00C6: // 00C6: Scroll display 6 lines down
                {
                    uint8_t n = 6;
                    int width = chip8.highRes ? HIGH_RES_WIDTH : LOW_RES_WIDTH;
                    int height = chip8.highRes ? HIGH_RES_HEIGHT : LOW_RES_HEIGHT;

                    if (n > 0) {
                        for (int y = height - 1; y >= n; --y) {
                            for (int x = 0; x < width; ++x) {
                                chip8.gfx[y * width + x] = chip8.gfx[(y - n) * width + x];
                            }
                        }
                        for (int y = 0; y < n; ++y) {
                            for (int x = 0; x < width; ++x) {
                                chip8.gfx[y * width + x] = 0;
                            }
                        }
                        chip8.drawFlag = true;
                    }
                }
                break;
                case 0x00DC: // 00DC: Scroll display 12 lines down
                {
                    uint8_t n = 12;
                    int width = chip8.highRes ? HIGH_RES_WIDTH : LOW_RES_WIDTH;
                    int height = chip8.highRes ? HIGH_RES_HEIGHT : LOW_RES_HEIGHT;

                    if (n > 0) {
                        for (int y = height - 1; y >= n; --y) {
                            for (int x = 0; x < width; ++x) {
                                chip8.gfx[y * width + x] = chip8.gfx[(y - n) * width + x];
                            }
                        }
                        for (int y = 0; y < n; ++y) {
                            for (int x = 0; x < width; ++x) {
                                chip8.gfx[y * width + x] = 0;
                            }
                        }
                        chip8.drawFlag = true;
                    }
                }
                break;
                case 0x00CC: // 00CC: Scroll display 12 lines down
                {
                    uint8_t n = 12;
                    int width = chip8.highRes ? HIGH_RES_WIDTH : LOW_RES_WIDTH;
                    int height = chip8.highRes ? HIGH_RES_HEIGHT : LOW_RES_HEIGHT;

                    if (n > 0) {
                        for (int y = height - 1; y >= n; --y) {
                            for (int x = 0; x < width; ++x) {
                                chip8.gfx[y * width + x] = chip8.gfx[(y - n) * width + x];
                            }
                        }
                        for (int y = 0; y < n; ++y) {
                            for (int x = 0; x < width; ++x) {
                                chip8.gfx[y * width + x] = 0;
                            }
                        }
                        chip8.drawFlag = true;
                    }
                }
                break;
                case 0x00FA: // 00FA: Compatible mode
                    chip8.compatMode = !chip8.compatMode;
                    break;
                case 0x00FE: // 00FE: LOW RES mode
                    chip8.highRes = false;
                    memset(chip8.gfx, 0, HIGH_RES_WIDTH * HIGH_RES_HEIGHT);
                    chip8.drawFlag = true;
                break;
                case 0x00FF: // 00FF: HIGH RES mode
                    chip8.highRes = true;
                    memset(chip8.gfx, 0, HIGH_RES_WIDTH * HIGH_RES_HEIGHT);
                    chip8.drawFlag = true;
                break;
                case 0x00FD: // 00FD: Exit the interpreter
                    running = false;
                    break;
                default:
                    printf("Unknown opcode: 0x%X\n", chip8.opcode);
                    break;
            }
            break;
        case 0x1000: // 1NNN: Jump to address NNN
            chip8.pc = chip8.opcode & 0x0FFF;
            break;
        case 0x2000: // 2NNN: Call subroutine at NNN
            chip8.stack[chip8.sp] = chip8.pc;
            chip8.sp++;
            chip8.pc = chip8.opcode & 0x0FFF;
            break;
        case 0x3000: // 3XNN: Skip next instruction if VX equals NN
            if (chip8.V[(chip8.opcode & 0x0F00) >> 8] == (chip8.opcode & 0x00FF)) {
                chip8.pc += 2;
            }
            break;
        case 0x4000: // 4XNN: Skip next instruction if VX does not equal NN
            if (chip8.V[(chip8.opcode & 0x0F00) >> 8] != (chip8.opcode & 0x00FF)) {
                chip8.pc += 2;
            }
            break;
        case 0x5000: // 5XY0: Skip next instruction if VX equals VY
            if (chip8.V[(chip8.opcode & 0x0F00) >> 8] == chip8.V[(chip8.opcode & 0x00F0) >> 4]) {
                chip8.pc += 2;
            }
            break;
        case 0x6000: // 6XNN: Set VX to NN
            chip8.V[(chip8.opcode & 0x0F00) >> 8] = chip8.opcode & 0x00FF;
            break;
        case 0x7000: // 7XNN: Add NN to VX
            chip8.V[(chip8.opcode & 0x0F00) >> 8] += chip8.opcode & 0x00FF;
            break;
        case 0x8000: // 8XYN: Perform operation based on N
            switch (chip8.opcode & 0x000F) {
                case 0x0000: // 8XY0: Set VX equal to VY
                    chip8.V[(chip8.opcode & 0x0F00) >> 8] = chip8.V[(chip8.opcode & 0x00F0) >> 4];
                    break;
                case 0x0001: // 8XY1: Set VX equal to VX OR VY
                    chip8.V[(chip8.opcode & 0x0F00) >> 8] |= chip8.V[(chip8.opcode & 0x00F0) >> 4];
                    if (!chip8.highRes) chip8.V[0xF] = 0;
                    break;
                case 0x0002: // 8XY2: Set VX equal to VX AND VY
                    chip8.V[(chip8.opcode & 0x0F00) >> 8] &= chip8.V[(chip8.opcode & 0x00F0) >> 4];
                    if (!chip8.highRes) chip8.V[0xF] = 0;
                    break;
                case 0x0003: // 8XY3: Set VX equal to VX XOR VY
                    chip8.V[(chip8.opcode & 0x0F00) >> 8] ^= chip8.V[(chip8.opcode & 0x00F0) >> 4];
                    if (!chip8.highRes) chip8.V[0xF] = 0;
                    break;
                case 0x0004: // 8XY4: Set VX equal to VX plus VY. In the case of an overflow VF is set to 1.
                    {
                        uint16_t sum = chip8.V[(chip8.opcode & 0x0F00) >> 8] + chip8.V[(chip8.opcode & 0x00F0) >> 4];
                        chip8.V[0xF] = sum > 0xFF;
                        chip8.V[(chip8.opcode & 0x0F00) >> 8] = sum & 0xFF;
                        //Overflow check
                        if (sum > 0xFF) {
                            chip8.V[0xF] = 1;
                        } else {
                            chip8.V[0xF] = 0;
                        }
                    }
                    break;
                case 0x0005: // 8XY5: Set VX equal to VX minus VY. VF is set to 0 when there's a borrow, and 1 when there isn't.
                    {
                        uint8_t vx = chip8.V[(chip8.opcode & 0x0F00) >> 8];
                        uint8_t vy = chip8.V[(chip8.opcode & 0x00F0) >> 4];
                        chip8.V[(chip8.opcode & 0x0F00) >> 8] = vx - vy;
                        // Check for borrow
                        chip8.V[0xF] = (vx >= vy) ? 0x01 : 0x00;
                    }
                break;
                case 0x0006: // 8XY6: Shift VX right by one. VF is set to the value of the least significant bit of VX before the shift.
                    if (chip8.compatMode) {
                        chip8.carry = (chip8.V[0xF] = chip8.V[(chip8.opcode & 0x0F00) >> 8] & 0x1);
                        chip8.V[(chip8.opcode & 0x0F00) >> 8] >>= 1;

                        chip8.V[0xF] = chip8.carry;
                    } else {
                        chip8.carry = (chip8.V[(chip8.opcode & 0x00F0) >> 4] & 0x1);
                        chip8.V[(chip8.opcode & 0x0F00) >> 8] = chip8.V[(chip8.opcode & 0x00F0) >> 4] >> 1;

                        chip8.V[0xF] = chip8.carry;
                    }
                break;
                case 0x0007: // 8XY7: Set VX equal to VY minus VX. VF is set to 1 if VY > VX. Otherwise 0
                    {
                        uint8_t vx = chip8.V[(chip8.opcode & 0x0F00) >> 8];
                        uint8_t vy = chip8.V[(chip8.opcode & 0x00F0) >> 4];
                        chip8.V[(chip8.opcode & 0x0F00) >> 8] = vy - vx;

                        // Check for borrow
                        chip8.V[0xF] = (vy >= vx) ? 0x01 : 0x00;
                    }
                break;
                case 0x000E: // 8XYE: Shift VX left by one. VF is set to the value of the most significant bit of VX before the shift.
                    if (chip8.compatMode) {
                        chip8.carry = ((chip8.V[(chip8.opcode & 0x0F00) >> 8] & 0x80) >> 7);
                        chip8.V[(chip8.opcode & 0x0F00) >> 8] <<= 1;

                        chip8.V[0xF] = chip8.carry;
                    } else {
                        chip8.carry = ((chip8.V[(chip8.opcode & 0x00F0) >> 4] & 0x80) >> 7);
                        chip8.V[(chip8.opcode & 0x0F00) >> 8] = chip8.V[(chip8.opcode & 0x00F0) >> 4] << 1;

                        chip8.V[0xF] = chip8.carry;
                    }
                break;
                default:
                    printf("Unknown opcode: 0x%X\n", chip8.opcode);
                    break;
            }
            break;
        case 0x9000: // 9XY0: Skip next instruction if VX does not equal VY
            if (chip8.V[(chip8.opcode & 0x0F00) >> 8] != chip8.V[(chip8.opcode & 0x00F0) >> 4]) {
                chip8.pc += 2;
            }
            break;
        case 0xA000: // ANNN: Set I to the address NNN
            chip8.I = chip8.opcode & 0x0FFF;
            break;
        case 0xB000: // BNNN: Jump to the address NNN plus V0
            {
                uint16_t address = chip8.opcode & 0x0FFF;
                chip8.pc = address + chip8.V[0];
            }
        break;
        case 0xC000: // CXNN: Set VX to a random number and NN
            chip8.V[(chip8.opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (chip8.opcode & 0x00FF);
            break;
        case 0xD000: // DXYN: Draw a sprite at position VX, VY with N bytes of sprite data starting at the address stored in I
        {
            const uint8_t x = chip8.V[(chip8.opcode & 0x0F00) >> 8];
            const uint8_t y = chip8.V[(chip8.opcode & 0x00F0) >> 4];
            const uint8_t height = chip8.opcode & 0x000F;
            const int width = chip8.highRes ? HIGH_RES_WIDTH : LOW_RES_WIDTH;
            const int displayHeight = chip8.highRes ? HIGH_RES_HEIGHT : LOW_RES_HEIGHT;

            chip8.V[0xF] = 0; // Reset VF

            for (int yline = 0; yline < height; yline++) {
                uint8_t pixel = chip8.memory[chip8.I + yline];

                for (int xline = 0; xline < 8; xline++) {
                    if ((pixel & (0x80 >> xline)) != 0) {
                        int xPos = (x + xline) % width;
                        int yPos = (y + yline) % displayHeight;

                        // Check for collision
                        if (chip8.gfx[yPos * width + xPos] == 1) {
                            chip8.V[0xF] = 1;
                        }
                        chip8.gfx[yPos * width + xPos] ^= 1;
                    }
                }
            }

            chip8.drawFlag = true; // Set flag to update the screen
        }
        break;

        case 0xE000:
            switch (chip8.opcode & 0x00FF) {
                case 0x009E: // EX9E: Skip next instruction if key with the value of VX is pressed
                    if (chip8.KC & ~(chip8.IK) & (1 << chip8.V[(chip8.opcode & 0x0F00) >> 8])) {
                        chip8.pc += 2;
                    }
                break;
                case 0x00A1: // EXA1: Skip next instruction if key with the value of VX is not pressed
                    if (!(chip8.KC & ~(chip8.IK) & (1 << chip8.V[(chip8.opcode & 0x0F00) >> 8]))) {
                        chip8.pc += 2;
                    }
                break;
                default:
                    printf("Unknown opcode: 0x%X\n", chip8.opcode);
                    break;
            }
            break;
        case 0xF000:
            switch (chip8.opcode & 0x00FF) {
                case 0x0007: // FX07: Set VX to the value of the delay timer
                    chip8.V[(chip8.opcode & 0x0F00) >> 8] = chip8.delay_timer;
                    break;
                case 0x000A: // FX0A: Wait for a key press, store the value of the key in Vx
                    chip8.waitingForKey = true;
                chip8.keyReg = (chip8.opcode & 0x0F00) >> 8;
                *interruptType = 2; // Set interrupt type for FX0A
                *SPEED = -(*SPEED); // Invert SPEED to quit early
                break;

                case 0x0015: // FX15: Set the delay timer to VX
                    chip8.delay_timer = chip8.V[(chip8.opcode & 0x0F00) >> 8];
                    break;
                case 0x0018: // FX18: Set the sound timer to VX
                    chip8.sound_timer = chip8.V[(chip8.opcode & 0x0F00) >> 8];
                    break;
                case 0x001E: // FX1E: Add VX to I
                    chip8.I += chip8.V[(chip8.opcode & 0x0F00) >> 8];
                    break;
                case 0x0029: // FX29: Set I to the location of the sprite for the character in VX
                    chip8.I = chip8.V[(chip8.opcode & 0x0F00) >> 8] * 0x5;
                    break;
                case 0x0033: // FX33: Store the binary-coded decimal representation of VX at the addresses I, I+1, and I+2
                    chip8.memory[chip8.I] = chip8.V[(chip8.opcode & 0x0F00) >> 8] / 100;
                    chip8.memory[chip8.I + 1] = (chip8.V[(chip8.opcode & 0x0F00) >> 8] / 10) % 10;
                    chip8.memory[chip8.I + 2] = (chip8.V[(chip8.opcode & 0x0F00) >> 8] % 10);
                    break;
                case 0x0055: // FX55: Store V0 to VX in memory starting at address I
                    for (int i = 0; i <= ((chip8.opcode & 0x0F00) >> 8); ++i) {
                        chip8.memory[chip8.I + i] = chip8.V[i];
                    }
                    if (!chip8.compatMode) {
                        chip8.I += ((chip8.opcode & 0x0F00) >> 8) + 1;
                    }
                    break;
                case 0x0065: // FX65: Fill V0 to VX with values from memory starting at address I
                    for (int i = 0; i <= ((chip8.opcode & 0x0F00) >> 8); ++i) {
                        chip8.V[i] = chip8.memory[chip8.I + i];
                    }
                    // If not in compat mode, increase I
                    if (!chip8.compatMode) {
                        chip8.I += ((chip8.opcode & 0x0F00) >> 8) + 1;
                    }
                    break;
                default:
                    printf("Unknown opcode: 0x%X\n", chip8.opcode);
                    break;
            }
            break;
        // Unknown opcode
        default:
            printf("Unknown opcode: 0x%X\n", chip8.opcode);
            break;
    }
}

// Update input values
void updateInput() {
    chip8.KP = chip8.KC;
    chip8.KC = 0;

    // Update KC based on current key states
    for (int i = 0; i < 16; ++i) {
        if (chip8.key[i]) {
            chip8.KC |= (1 << i);
        }
    }

    // Update IK to detect key releases
    chip8.IK = chip8.KP & ~chip8.KC;
}

// Handle interrupts
void handleInterrupts(int *SPEED, int *interruptType) {
    if (*interruptType == 2) { // FX0A interrupt
        uint16_t keyPresses = chip8.KC & ~chip8.IK;
        if (keyPresses) {
            chip8.V[chip8.keyReg] = log2(keyPresses & -keyPresses);
            chip8.IK |= keyPresses;
            *interruptType = -1; // Clear interrupt
            *SPEED = abs(*SPEED); // Restore SPEED
        }
    }
}

// Update timers
void updateTimers() {
    if (chip8.delay_timer > 0) {
        --chip8.delay_timer;
    }
    if (chip8.sound_timer > 0) {
        // Play the beep sound
        soundBeep();
        --chip8.sound_timer;
    } else {
        // Stop the beep sound
        SDL_ClearQueuedAudio(chip8.beepDevice);
        SDL_PauseAudioDevice(chip8.beepDevice, 1);
    }
}

void drawGfx(SDL_Renderer *renderer) {
    // Set background color and clear screen
    SDL_SetRenderDrawColor(renderer, chip8.theme.bgColor.r, chip8.theme.bgColor.g, chip8.theme.bgColor.b, chip8.theme.bgColor.a);
    SDL_RenderClear(renderer);

    // Set foreground color
    SDL_SetRenderDrawColor(renderer, chip8.theme.fgColor.r, chip8.theme.fgColor.g, chip8.theme.fgColor.b, chip8.theme.fgColor.a);

    int width = chip8.highRes ? HIGH_RES_WIDTH : LOW_RES_WIDTH;
    int height = chip8.highRes ? HIGH_RES_HEIGHT : LOW_RES_HEIGHT;
    int pixelSize = chip8.highRes ? 5 : 10; // Adjust pixel size for high-res

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (chip8.gfx[y * width + x] == 1) {
                int drawX = (x * pixelSize) % (width * pixelSize);
                int drawY = (y * pixelSize) % (height * pixelSize);

                // Draw pixel
                SDL_Rect pixelRect = { drawX, drawY, pixelSize, pixelSize };
                SDL_RenderFillRect(renderer, &pixelRect);
            }
        }
    }
    SDL_RenderPresent(renderer);
}

// Initialize audio
void initAudio() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("SDL2 audio could not be initialized. %s\n", SDL_GetError());
        return;
    }

    // Set audio specifications
    chip8.beepSpec.freq = 44100;
    chip8.beepSpec.format = AUDIO_F32SYS;
    chip8.beepSpec.channels = 1;
    chip8.beepSpec.samples = 2048;
    chip8.beepSpec.callback = NULL;

    // Open audio device
    chip8.beepDevice = SDL_OpenAudioDevice(NULL, 0, &chip8.beepSpec, NULL, 0);
    if (chip8.beepDevice == 0) {
        printf("SDL2 audio device could not be opened for playback. %s\n", SDL_GetError());
    }
}

// Play beep sound
void soundBeep() {
    if (chip8.beepDevice == 0) {
        return; // Audio device not initialized
    }

    // Play beep sound
    if (chip8.sound_timer > 0) {
        int sampleCount = chip8.beepSpec.freq;
        float* buffer = (float*)malloc(sizeof(float) * sampleCount);

        for (int i = 0; i < sampleCount; ++i) {
            buffer[i] = sin(2 * M_PI * 440 * i / chip8.beepSpec.freq);
        }

        SDL_QueueAudio(chip8.beepDevice, buffer, sampleCount * sizeof(float));
        SDL_PauseAudioDevice(chip8.beepDevice, 0);

        free(buffer);
    } else {
        // Stop the beep sound
        SDL_ClearQueuedAudio(chip8.beepDevice);
        SDL_PauseAudioDevice(chip8.beepDevice, 1);
    }
}