#include "../include/app_state.hpp"

AppState::AppState(unsigned int scale) : 
    winScale{scale},
    window(winWidth*winScale, winHeight*winScale), 
    context(window.getWindow(), winWidth*winScale, winHeight*winScale),
    guiState(winWidth*winScale, winHeight*winScale),
    fluidRenderer(winWidth*winScale, winHeight*winScale)
{
}

void AppState::beginLoop(){
    tStart = std::chrono::high_resolution_clock::now();
}

void AppState::mainLoop(){
    // Handle event queue
    while (SDL_PollEvent(&event)){
        handleEvents(event);
    }
    // Get duration of current frame in microseconds
    tNow = std::chrono::high_resolution_clock::now();
    unsigned int frameTime = std::chrono::duration_cast<std::chrono::microseconds>(tNow - tStart).count();

    // Cap frame length at 250 ms in case of lag, thereby to
    // prevent simulation and display getting too out of sync
    if (frameTime > 250000){
        frameTime = 250000;
        std::cout << "Frame hit maximum duration!\n";
    }

    tStart = tNow;
    frame(frameTime);
}

void AppState::handleEvents(SDL_Event const&  event){
    switch(event.type){
        case SDL_QUIT:
            quitApp();
            break;
        case SDL_KEYDOWN:
            switch(event.key.keysym.scancode){
                case SDL_SCANCODE_F11:
                    window.toggleFullScreen();
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    guiState.handleEvents(event);
    fluidRenderer.handleEvents(event);

}

void AppState::frame(unsigned int frameTime){ // Move to app state frame()
    // Clear buffer
    glClearColor(0.0f, 0.0f, 0.0f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Draw fluid
    fluidRenderer.frame(frameTime);

    // Draw GUI
    guiState.frame(frameTime);
    
    // Swap buffers
    SDL_GL_SwapWindow(window.getWindow());

    // Update FPS counter
    window.frame(frameTime);
}

void AppState::quitApp(){
    quit = true;
    #ifdef __EMSCRIPTEN__
    //emscripten_cancel_main_loop();
    #endif
}