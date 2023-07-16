#ifndef _JULIA_WINDOW_HPP_
#define _JULIA_WINDOW_HPP_

#include <SDL.h>
#include <SDL_opengl.h>

#include <string>

class Window{
public:
    Window(unsigned int width, unsigned int height);
    ~Window();
    Window(const Window& other) = delete;
    Window &operator=(const Window& other) = delete;
    Window(Window&& other) = delete;
    Window &operator=(Window&& other) = delete;
    // Utility functions
    SDL_Window* getWindow();
    void toggleFullScreen();
    void frame(unsigned int frameTime);
    unsigned int const winWidth = 640;
    unsigned int const winHeight = 480;
private:
    SDL_Window* window = nullptr;
    bool fullScreen = false;
    Uint32 winFlags = SDL_WINDOW_OPENGL;
    // FPS counter
    unsigned int accumulatedFrameTime = 0;
    unsigned int numFrames = 0;
};

#endif