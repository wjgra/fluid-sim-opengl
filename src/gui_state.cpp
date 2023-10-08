#include "../include/gui_state.hpp"

GUIState::GUIState(unsigned int width, unsigned int height) : width{width}, height{height}{
}

void GUIState::frame(__attribute__((unused)) unsigned int frameTime){
     textRen.drawString("FLUID SIMULATION", 10.0f, 0.5f,0.5f);//width * 0.1f, height * 0.1f);
}

void GUIState::handleEvents(__attribute__((unused)) const SDL_Event& event){
}

