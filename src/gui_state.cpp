#include "gui_state.hpp"

GUIState::GUIState(unsigned int width, unsigned int height) : m_textRen(width, height){
}

bool GUIState::successfullyInitialised() const{
     return m_textRen.successfullyInitialised();
}

void GUIState::frame(){
     m_textRen.drawString("FLUID SIMULATION", 10.0f, 0.5f,0.5f);
}
