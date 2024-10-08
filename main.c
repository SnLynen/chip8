#include <SDL2/SDL_events.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <time.h>

#include "chip8.h"

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

bool initSdl(){
  if((SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)==-1)) {
    printf("Could not initialize SDL: %s", SDL_GetError());
    return 0;
  }
  return 1;
}

int loadRom(char *rom){
  FILE *file = fopen(rom, "rb");
  if(file == NULL){
    printf("Failed to open file: %s\n", rom);
    return -1;
  }

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  rewind(file);

  char *buffer = (char*)malloc(sizeof(char) * size);
  if(buffer == NULL){
    printf("Failed to allocate memory\n");
    return -1;
  }

  size_t result = fread(buffer, 1, size, file);
  if(result != size){
    printf("Failed to read file\n");
    return -1;
  }

  if((4096-512) > size){
    for(int i=0; i<size; i++){
      memory[i + 512] = buffer[i];
    }
  }else{
    printf("ROM too big for memory\n");
    return -1;
  }

  fclose(file);
  free(buffer);
  return 0;
}


//initialize chip8 values to default
void initChip8(){
  pc = 0x200;
  I = 0;
  sp = 0;
  delay_timer = 0;
  sound_timer = 0;
  memset(memory, 0, 4096);
  memset(V, 0, sizeof(V));
  memset(stack, 0, sizeof(stack));
  memset(gfx, 0, sizeof(gfx));

  //Load fontset
  for(int i=0; i<80; i++){
    memory[i] = chip8_fontset[i];
  }
}

void draw(SDL_Renderer *renderer){
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

  for(int y=0; y<32; y++){
    for(int x=0; x<64; x++){
      if(gfx[(y*64) + x] == 1){
        SDL_Rect rect = {x*10, y*10, 10, 10};
        SDL_RenderFillRect(renderer, &rect);
      }
    }
  }

  SDL_RenderPresent(renderer);
}

//Cycle chip8
void emuCycle() {
    opcode = memory[pc] << 8 | memory[pc + 1];
    pc += 2;

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x000F) {
                case 0x0000: // 0x00E0: Clears the screen
                    memset(gfx, 0, sizeof(gfx));
                    drawFlag = true;
                    break;
                case 0x000E: // 0x00EE: Returns from subroutine
                    --sp;
                    pc = stack[sp];
                    break;
                default:
                    printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
                    break;
            }
            break;

        case 0x1000: // 0x1NNN: Jumps to address NNN
            pc = opcode & 0x0FFF;
            break;

        case 0x2000: // 0x2NNN: Calls subroutine at NNN
            stack[sp] = pc;
            ++sp;
            pc = opcode & 0x0FFF;
            break;

        case 0x3000: // 0x3XNN: Skips the next instruction if VX equals NN
            if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
                pc += 2;
            }
            break;

        case 0x4000: // 0x4XNN: Skips the next instruction if VX doesn't equal NN
            if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
                pc += 2;
            }
            break;

        case 0x5000: // 0x5XY0: Skips the next instruction if VX equals VY
            if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4]) {
                pc += 2;
            }
            break;

        case 0x6000: // 0x6XNN: Sets VX to NN
            V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            break;

        case 0x7000: // 0x7XNN: Adds NN to VX
            V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            break;

        case 0x8000:
            switch (opcode & 0x000F) {
                case 0x0000: // 0x8XY0: Sets VX to the value of VY
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
                    break;
                case 0x0001: // 0x8XY1: Sets VX to VX or VY
                    V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
                    break;
                case 0x0002: // 0x8XY2: Sets VX to VX and VY
                    V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
                    break;
                case 0x0003: // 0x8XY3: Sets VX to VX xor VY
                    V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
                    break;
                case 0x0004: // 0x8XY4: Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't
                    if (V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8])) {
                        V[0xF] = 1; // carry
                    } else {
                        V[0xF] = 0;
                    }
                    V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
                    break;
                case 0x0005: // 0x8XY5: VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't
                    if (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8]) {
                        V[0xF] = 0; // borrow
                    } else {
                        V[0xF] = 1;
                    }
                    V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
                    break;
                case 0x0006: // 0x8XY6: Shifts VX right by one. VF is set to the value of the least significant bit of VX before the shift
                    V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
                    V[(opcode & 0x0F00) >> 8] >>= 1;
                    break;
                case 0x0007: // 0x8XY7: Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't
                    if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4]) {
                        V[0xF] = 0; // borrow
                    } else {
                        V[0xF] = 1;
                    }
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
                    break;
                case 0x000E: // 0x8XYE: Shifts VX left by one. VF is set to the value of the most significant bit of VX before the shift
                    V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
                    V[(opcode & 0x0F00) >> 8] <<= 1;
                    break;
                default:
                    printf("Unknown opcode [0x8000]: 0x%X\n", opcode);
                    break;
            }
            break;

        case 0x9000: // 0x9XY0: Skips the next instruction if VX doesn't equal VY
            if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4]) {
                pc += 2;
            }
            break;

        case 0xA000: // 0xANNN: Sets I to the address NNN
            I = opcode & 0x0FFF;
            break;

        case 0xB000: // 0xBNNN: Jumps to the address NNN plus V0
            pc = (opcode & 0x0FFF) + V[0];
            break;

        case 0xC000: // 0xCXNN: Sets VX to a random number and NN
            V[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0x00FF);
            break;

        case 0xD000: { // 0xDXYN: Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels
            unsigned short x = V[(opcode & 0x0F00) >> 8];
            unsigned short y = V[(opcode & 0x00F0) >> 4];
            unsigned short height = opcode & 0x000F;
            unsigned short pixel;

            V[0xF] = 0;
            for (int yline = 0; yline < height; yline++) {
                pixel = memory[I + yline];
                for (int xline = 0; xline < 8; xline++) {
                    if ((pixel & (0x80 >> xline)) != 0) {
                        if (gfx[(x + xline + ((y + yline) * 64))] == 1) {
                            V[0xF] = 1;
                        }
                        gfx[x + xline + ((y + yline) * 64)] ^= 1;
                    }
                }
            }

            drawFlag = true;
        }
        break;

        case 0xE000:
            switch (opcode & 0x00FF) {
                case 0x009E: // 0xEX9E: Skips the next instruction if the key stored in VX is pressed
                    if (keypad[V[(opcode & 0x0F00) >> 8]] != 0) {
                        pc += 2;
                    }
                    break;
                case 0x00A1: // 0xEXA1: Skips the next instruction if the key stored in VX isn't pressed
                    if (keypad[V[(opcode & 0x0F00) >> 8]] == 0) {
                        pc += 2;
                    }
                    break;
                default:
                    printf("Unknown opcode [0xE000]: 0x%X\n", opcode);
                    break;
            }
            break;

        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x0007: // 0xFX07: Sets VX to the value of the delay timer
                    V[(opcode & 0x0F00) >> 8] = delay_timer;
                    break;
                case 0x000A: { // 0xFX0A: A key press is awaited, and then stored in VX
                    bool keyPress = false;
                    for (int i = 0; i < 16; ++i) {
                        if (keypad[i] != 0) {
                            V[(opcode & 0x0F00) >> 8] = i;
                            keyPress = true;
                        }
                    }

                    if (!keyPress) {
                        pc -= 2;
                        return;
                    }
                }
                break;
                case 0x0015: // 0xFX15: Sets the delay timer to VX
                    delay_timer = V[(opcode & 0x0F00) >> 8];
                    break;
                case 0x0018: // 0xFX18: Sets the sound timer to VX
                    sound_timer = V[(opcode & 0x0F00) >> 8];
                    break;
                case 0x001E: // 0xFX1E: Adds VX to I
                    if (I + V[(opcode & 0x0F00) >> 8] > 0xFFF) {
                        V[0xF] = 1;
                    } else {
                        V[0xF] = 0;
                    }
                    I += V[(opcode & 0x0F00) >> 8];
                    break;
                case 0x0029: // 0xFX29: Sets I to the location of the sprite for the character in VX
                    I = V[(opcode & 0x0F00) >> 8] * 0x5;
                    break;
                case 0x0033: // 0xFX33: Stores the binary-coded decimal representation of VX at the addresses I, I plus 1, and I plus 2
                    memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
                    memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
                    memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 10);
                    break;
                case 0x0055: // 0xFX55: Stores V0 to VX in memory starting at address I
                    for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i) {
                        memory[I + i] = V[i];
                    }
                    I += ((opcode & 0x0F00) >> 8) + 1;
                    break;
                case 0x0065: // 0xFX65: Fills V0 to VX with values from memory starting at address I
                    for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i) {
                        V[i] = memory[I + i];
                    }
                    I += ((opcode & 0x0F00) >> 8) + 1;
                    break;
                default:
                    printf("Unknown opcode [0xF000]: 0x%X\n", opcode);
                    break;
            }
            break;

        default:
            printf("Unknown opcode: 0x%X\n", opcode);
            break;
    }

    if (delay_timer > 0) {
        --delay_timer;
    }

    if (sound_timer > 0) {
        if (sound_timer == 1) {
            printf("BEEP!\n");
        }
        --sound_timer;
    }
}

int main(int argc, char *argv[])
{
  //Check if SDL is initialized correctly
  if(!initSdl()){
    return EXIT_FAILURE;
  }

  //Create window
  SDL_Window *window = SDL_CreateWindow("CHIP8 Emulator",
                                        SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED,
                                        640, 320,
                                        0);
  
  //Check if window failed
  if(!window){
    printf("Failed to create window: %s\n", SDL_GetError());
    return false;
  }

renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if(!renderer){
    printf("Failed to create renderer: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    return false;
  }

  //Check if rom is provided
  if (argc < 2) {
    printf("Do: %s <romfile>\n", argv[0]);
    return EXIT_FAILURE;
  }
  
  //initialize chip8 
  initChip8();

  //Load rom
  if(loadRom(argv[1]) != 0){
    return EXIT_FAILURE;
  }

  //Check events
  SDL_Event event;
  bool running = true;
  while(running){
    while(SDL_PollEvent(&event)){
      //If window is closed stop loop
      if(event.type==SDL_QUIT){
        running = false;
      }

        //If key is pressed
        switch(event.type){
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym){
                    case SDLK_1:
                        keypad[0x1] = 1;
                        break;
                    case SDLK_2:
                        keypad[0x2] = 1;
                        break;
                    case SDLK_3:
                        keypad[0x3] = 1;
                        break;
                    case SDLK_4:
                        keypad[0xC] = 1;
                        break;
                    case SDLK_q:
                        keypad[0x4] = 1;
                        break;
                    case SDLK_w:
                        keypad[0x5] = 1;
                        break;
                    case SDLK_e:
                        keypad[0x6] = 1;
                        break;
                    case SDLK_r:
                        keypad[0xD] = 1;
                        break;
                    case SDLK_a:
                        keypad[0x7] = 1;
                        break;
                    case SDLK_s:
                        keypad[0x8] = 1;
                        break;
                    case SDLK_d:
                        keypad[0x9] = 1;
                        break;
                    case SDLK_f:
                        keypad[0xE] = 1;
                        break;
                    case SDLK_z:
                        keypad[0xA] = 1;
                        break;
                    case SDLK_x:
                        keypad[0x0] = 1;
                        break;
                    case SDLK_c:
                        keypad[0xB] = 1;
                        break;
                    case SDLK_v:
                        keypad[0xF] = 1;
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }

        //If key is released
        switch(event.type){
            case SDL_KEYUP:
                switch(event.key.keysym.sym){
                    case SDLK_1:
                        keypad[0x1] = 0;
                        break;
                    case SDLK_2:
                        keypad[0x2] = 0;
                        break;
                    case SDLK_3:
                        keypad[0x3] = 0;
                        break;
                    case SDLK_4:
                        keypad[0xC] = 0;
                        break;
                    case SDLK_q:
                        keypad[0x4] = 0;
                        break;
                    case SDLK_w:
                        keypad[0x5] = 0;
                        break;
                    case SDLK_e:
                        keypad[0x6] = 0;
                        break;
                    case SDLK_r:
                        keypad[0xD] = 0;
                        break;
                    case SDLK_a:
                        keypad[0x7] = 0;
                        break;
                    case SDLK_s:
                        keypad[0x8] = 0;
                        break;
                    case SDLK_d:
                        keypad[0x9] = 0;
                        break;
                    case SDLK_f:
                        keypad[0xE] = 0;
                        break;
                    case SDLK_z:
                        keypad[0xA] = 0;
                        break;
                    case SDLK_x:
                        keypad[0x0] = 0;
                        break;
                    case SDLK_c:
                        keypad[0xB] = 0;
                        break;
                    case SDLK_v:
                        keypad[0xF] = 0;
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }

    }

    //Cycle chip8
    emuCycle();

        //Draw if flag is set
        if (drawFlag) {
            drawFlag = false;
            draw(renderer);
        }
      SDL_Delay(16);
  }

  //Destroy window and renderer
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return EXIT_SUCCESS;
}
