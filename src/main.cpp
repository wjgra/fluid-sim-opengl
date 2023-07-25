//#include "../include/glad/glad.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_main.h>

#include <iostream>
#include <chrono>

#include "../include/app_state.hpp"

#ifdef __EMSCRIPTEN__
// Callback function to enable setting of Emscripten loop
void mainLoopCallback(void* appState){
    static_cast<AppState*>(appState)->mainLoop();
}
#endif

int main(){
    try{
        // Initialise window, openGL context and game state
        constexpr int winScale = 2;
        AppState appState(winScale);

        appState.beginLoop();

        #ifndef __EMSCRIPTEN__
        while (!appState.quit){
            appState.mainLoop();
        }
        #else
        emscripten_set_main_loop_arg(&mainLoopCallback, &appState, 0, 1);
        #endif
    }
    catch (std::string exception){
        // Exit if failed to initialise appState
        std::cerr << exception << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// To do:
// - Particle effects