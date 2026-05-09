#include "../include/input/input.hpp"
#include "../include/input/controller.hpp"
#include <SDL2/SDL.h>
#include <iostream>

namespace md {

InputManager::InputManager() = default;

InputManager::~InputManager() {
    cleanup_sdl();
}

void InputManager::init() {
    init_sdl();
    scan_devices();
}

void InputManager::init_sdl() {
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "SDL init error: " << SDL_GetError() << "\n";
    }
}

void InputManager::cleanup_sdl() {
    SDL_Quit();
}

void InputManager::update() {
    SDL_GameControllerUpdate();
    
    for (auto& ctrl : controllers) {
        if (ctrl) {
            ctrl->update();
        }
    }
}

void InputManager::scan_devices() {
    int num = SDL_NumJoysticks();
    
    for (int i = 0; i < std::min(num, 2); i++) {
        if (SDL_IsGameController(i)) {
            sdl_controllers[i] = SDL_GameControllerOpen(i);
            std::cout << "Controller " << i << " connected: " 
                      << SDL_GameControllerName(sdl_controllers[i]) << "\n";
            
            controllers[i] = std::make_shared<MDController>();
        }
    }
}

ButtonState InputManager::get_button(int port, Button btn) const {
    if (port >= 0 && port < 2 && controllers[port]) {
        return controllers[port]->get_md_button(btn);
    }
    return ButtonState{};
}

float InputManager::get_axis(int port, Axis axis) const {
    if (port >= 0 && port < 2 && controllers[port]) {
        return controllers[port]->get_state().axes[axis];
    }
    return 0.0f;
}

void InputManager::load_profile(const std::string& /*game_id*/) {}
void InputManager::save_profile(const std::string& /*game_id*/) {}

} // namespace md
