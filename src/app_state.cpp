#include "app_state.hpp"

AppState::AppState(unsigned int w, unsigned int h, unsigned int scale) : 
    m_windowDisplayScale{scale},
    m_notionalWindowWidth{w},
    m_notionalWindowHeight{h},
    m_quitApplication{false},
    m_window(m_notionalWindowWidth * m_windowDisplayScale, m_notionalWindowHeight * m_windowDisplayScale), 
    m_context(m_window.getWindow(), m_notionalWindowWidth* m_windowDisplayScale, m_notionalWindowHeight * m_windowDisplayScale),
    m_fluid(m_notionalWindowWidth * m_windowDisplayScale, m_notionalWindowHeight * m_windowDisplayScale),
    m_guiState(m_notionalWindowWidth, m_notionalWindowHeight)
{
}

bool AppState::successfullyInitialised() const{
    bool success = m_window.successfullyInitialised();
    success &= m_context.successfullyInitialised();
    success &= m_fluid.successfullyInitialised();
    success &= m_guiState.successfullyInitialised();
    return success;
}

void AppState::beginLoop(){
    m_tStart = std::chrono::high_resolution_clock::now();
}

void AppState::mainLoop(){
    SDL_Event event;
    while (SDL_PollEvent(&event)){
        handleEvents(event);
    }

    m_tNow = std::chrono::high_resolution_clock::now();
    unsigned int frameTime = std::chrono::duration_cast<std::chrono::microseconds>(m_tNow - m_tStart).count();
    m_tStart = m_tNow;

    // Cap frame length to 250 ms
    if (frameTime > 250000){
        frameTime = 250000;
    }
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
                    m_window.toggleFullScreen();
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    m_fluid.handleEvents(event);
}

void AppState::frame(unsigned int frameTime){
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_fluid.frame(frameTime);
    m_guiState.frame();
    SDL_GL_SwapWindow(m_window.getWindow());
    m_window.frame(frameTime);
}

void AppState::quitApp(){
    m_quitApplication = true;
}

bool AppState::timeToQuit() const{
    return m_quitApplication;
}