#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_main.h>

#include <iostream>
#include <chrono>

#include "app_state.hpp"

#ifdef __EMSCRIPTEN__
void mainLoopCallback(void* appState){
    static_cast<AppState*>(appState)->mainLoop();
}
#endif

// #define FORCE_DEDICATED_NVIDIA_GPU
#ifdef FORCE_DEDICATED_NVIDIA_GPU
extern "C" 
{
  __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}
#endif

int main(){
    AppState appState(640, 480, 2);
    if (!appState.successfullyInitialised()){
        return EXIT_FAILURE;
    }
    appState.beginLoop();

    #ifndef __EMSCRIPTEN__
    while (!appState.timeToQuit()){
        appState.mainLoop();
    }
    #else
    emscripten_set_main_loop_arg(&mainLoopCallback, &appState, 0, 1);
    #endif

    return EXIT_SUCCESS;
}