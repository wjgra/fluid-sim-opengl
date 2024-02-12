#include "../include/context.hpp"

Context::Context(SDL_Window* window, unsigned int width, unsigned int height) : viewportWidth{width}, viewportHeight{height} {
    try{
        // Create OpenGL context
        context = SDL_GL_CreateContext(window);
        if (!context){
            throw "Failed to create OpenGL context";
        }
        std::cout << "OpenGL loaded\n";
        
        #ifndef __EMSCRIPTEN__
        // Load OpenGL functions with GLAD
        gladLoadGLLoader(SDL_GL_GetProcAddress);
        #endif

        // Display device information
        printf("Vendor:   %s\n", glGetString(GL_VENDOR));
        printf("Renderer: %s\n", glGetString(GL_RENDERER));
        printf("Version:  %s\n", glGetString(GL_VERSION));

        // Set v-sync
        SDL_GL_SetSwapInterval(useVsync);

        glViewport(0, 0, viewportWidth, viewportHeight);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_successfullyInitialised = true;
    }
    catch(std::exception const& e){
        std::cerr << "[ERROR]: " << e.what() << "\n";
        m_successfullyInitialised = false;
    }
}

Context::~Context(){
    SDL_GL_DeleteContext(context);
}

SDL_GLContext Context::getContext() const{
    return context;
}

bool Context::successfullyInitialised() const{
    return m_successfullyInitialised;
}