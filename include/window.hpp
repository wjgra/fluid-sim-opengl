#ifndef _FLUID_WINDOW_HPP_
#define _FLUID_WINDOW_HPP_

#include <SDL.h>
#include <SDL_opengl.h>

#include <string>
#include <stdexcept>
#include <iostream>

class Window{
    Uint32 const m_windowCreationFlags = SDL_WINDOW_OPENGL;
public:
    Window(unsigned int width, unsigned int height);
    ~Window();
    Window(Window const&) = delete;
    Window(Window const&&) = delete;
    Window& operator=(Window const&) = delete;
    Window& operator=(Window const&&) = delete;
    bool successfullyInitialised() const;
    SDL_Window* getWindow() const;
    void toggleFullScreen();
    void frame(unsigned int frameTime);
    unsigned int const m_winWidth;
    unsigned int const m_winHeight;
private:
    SDL_Window* m_window = nullptr;
    bool m_fullScreen = false;
    unsigned int m_timeSinceLastUpdate = 0;
    unsigned int m_numberOfFramesSinceLastUpdate = 0;
    bool m_successfullyInitialised = false;
};

#endif