#include "../include/window.hpp"

Window::Window(unsigned int width, unsigned int height) : m_winWidth{width}, m_winHeight{height}{
    try{
        // Initialise SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0){
            throw std::runtime_error("Failed to initialise SDL");
        }

        #ifndef __EMSCRIPTEN__
        SDL_GL_LoadLibrary(nullptr); 
        #endif

        SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
        #ifndef __EMSCRIPTEN__
        // Request an OpenGL 3.3 context
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        #endif
        
        // Request a depth buffer
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

        // Create window
        m_window = SDL_CreateWindow(
            "Fluid Simulation", 
            SDL_WINDOWPOS_CENTERED, 
            SDL_WINDOWPOS_CENTERED, 
            m_winWidth, 
            m_winHeight, 
            m_windowCreationFlags);

        if (!m_window){
            throw std::runtime_error("Failed to create window");
        }
        m_successfullyInitialised = true;
    }
    catch (std::exception const& e){
        std::cerr << "[ERROR]: " << e.what() << "\n";
        m_successfullyInitialised = false;
    }

}

Window::~Window(){
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

bool Window::successfullyInitialised() const{
    return m_successfullyInitialised;
}

SDL_Window* Window::getWindow() const{
    return m_window;
}

void Window::toggleFullScreen(){
    m_fullScreen = !m_fullScreen;
    if (m_fullScreen){
        SDL_SetWindowFullscreen(m_window, m_windowCreationFlags | SDL_WINDOW_FULLSCREEN);
    }
    else{
        SDL_SetWindowFullscreen(m_window, m_windowCreationFlags);
    }
}

void Window::frame(unsigned int frameTime){
    // Calculate average frame length and FPS for display in title bar
    m_timeSinceLastUpdate += frameTime;
    ++m_numberOfFramesSinceLastUpdate;
    if (m_timeSinceLastUpdate > 1000000){ // every second
        float avgFrameTime = (float)m_timeSinceLastUpdate / ((float)m_numberOfFramesSinceLastUpdate*1000.0f); // in ms
        int fps = int(1000.0f/avgFrameTime);
        SDL_SetWindowTitle(m_window, 
            std::string("Fluid Simulation - FPS: "+std::to_string(fps)+" ("+std::to_string(avgFrameTime)+" ms)").c_str());
        m_timeSinceLastUpdate = 0;
        m_numberOfFramesSinceLastUpdate = 0;
    }
}