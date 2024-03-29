#include <SDL2/SDL.h>
#include <iostream>
#include "types.h"
// #include "logger.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

class Display
{
    MCU *mcus;
    unsigned int width;
    unsigned int height;
    unsigned int mcuWidthReal;
    unsigned int mcuHeightReal;
    SDL_Window *window;
    SDL_Renderer *renderer;

public:
    Display(MCU *mcus, unsigned int width, unsigned int height, unsigned mcuWidthReal, unsigned mcuHeightReal)
    {
        this->mcus = mcus;
        this->height = height;
        this->width = width;
        this->mcuHeightReal = mcuHeightReal;
        this->mcuWidthReal = mcuWidthReal;
        // Initialize SDL with video support
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
            return;
        }

        // Create window
        this->window = SDL_CreateWindow("SDL Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, this->width, this->height, SDL_WINDOW_SHOWN);
        if (window == nullptr)
        {
            std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return;
        }

        // Create renderer
        this->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (renderer == nullptr)
        {
            std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return;
        }
    }

    void display()
    {
        for (int y = this->height -1; y >=0; --y)
        {
            const int mcuRow = y / 8;
            const int pixelRow = y % 8;
            for (int x = 0; x < this->width; x++)
            {
                const int mcuColumn = x / 8;
                const int pixelColumn = x % 8;
                const int mcuIndex = mcuRow * this->mcuWidthReal + mcuColumn;
                const int pixelIndex = pixelRow * 8 + pixelColumn;
                SDL_Rect rect = {x, (this->height - y), SCREEN_WIDTH / this->width, SCREEN_HEIGHT / this->height};
                // SDL_Rect rect = {;
                SDL_SetRenderDrawColor(renderer, this->mcus[mcuIndex].y[pixelIndex], this->mcus[mcuIndex].cr[pixelIndex], this->mcus[mcuIndex].cb[pixelIndex], 0xFF); // Red color
                // SDL_SetRenderDrawColor(renderer, 0xff,0xff,0xff,0xff);
                SDL_RenderFillRect(renderer, &rect);
                // break;
            }
            // break;
        }

        // Update screen
        SDL_RenderPresent(renderer);

        // Wait for 5 seconds
        SDL_Delay(5000);
    }
    void dispose()
    {

        // Destroy window and quit SDL
        SDL_DestroyRenderer(this->renderer);
        SDL_DestroyWindow(this->window);
        SDL_Quit();
    }
};
