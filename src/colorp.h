//
// Created by ariel on 10/11/24.
//

#ifndef COLORP_H
#define COLORP_H

#include <SDL2/SDL.h>

struct Theme {
    SDL_Color bgColor;
    SDL_Color fgColor;
};

struct Theme themes[] = {
    { {0, 0, 0, 255}, {255, 255, 255, 255} }, // Black background, White foreground
    { {255, 255, 255, 255}, {0, 0, 0, 255} }, // White background, Black foreground
    { {104, 55, 43, 255}, {255, 255, 255, 255} }, // Dark red background, White foreground
    { {112, 164, 178, 255}, {0, 0, 0, 255} }, // Cyan background, Black foreground
    { {111, 61, 134, 255}, {255, 255, 255, 255} }, // Purple background, White foreground
    { {88, 141, 67, 255}, {0, 0, 0, 255} }, // Green background, Black foreground
    { {53, 40, 121, 255}, {255, 255, 255, 255} }, // Dark blue background, White foreground
    { {184, 199, 111, 255}, {0, 0, 0, 255} }, // Light yellow background, Black foreground
    { {111, 79, 37, 255}, {255, 255, 255, 255} }, // Dark brown background, White foreground
    { {67, 57, 0, 255}, {255, 255, 255, 255} }, // Dark olive green background, White foreground
    { {154, 103, 89, 255}, {0, 0, 0, 255} }, // Light orange background, Black foreground
    { {68, 68, 68, 255}, {255, 255, 255, 255} }, // Dark grey background, White foreground
    { {108, 108, 108, 255}, {0, 0, 0, 255} }, // Grey background, Black foreground
    { {172, 172, 172, 255}, {0, 0, 0, 255} }, // Light grey background, Black foreground
    // Add more theme following the format provided: { {Red, Green, Blue, Alpha}, {Red, Green, Blue, Alpha} } 1st color is background, 2nd color is foreground
};

#endif // COLORP_H