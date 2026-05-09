#ifndef INPUT_HPP
#define INPUT_HPP

#include "controller.hpp"
#include <vector>
#include <memory>
#include <unordered_map>
#include <SDL2/SDL.h>

namespace md {

// Менеджер ввода
class InputManager {
public:
    InputManager();
    ~InputManager();

    void init();
    void update();  // Вызывать каждый кадр
    
    // Регистрация контроллеров
    void scan_devices();
    std::shared_ptr<Controller> get_controller(int port) const { return controllers[port]; }
    
    // Маппинг кнопок
    ButtonState get_button(int port, Button btn) const;
    float get_axis(int port, Axis axis) const;
    
    // Загрузка/сохранение профилей
    void load_profile(const std::string& game_id);
    void save_profile(const std::string& game_id);

private:
    std::array<std::shared_ptr<Controller>, 2> controllers;  // 2 порта MD
    
    // SDL GameController для современных геймпадов
    SDL_GameController* sdl_controllers[4] = {nullptr};
    
    // Профили маппинга
    struct Profile {
        std::array<int, BTN_MAX> md_to_modern;  // индексы кнопок
        float deadzone = 0.1f;
        float rumble_intensity = 1.0f;
    };
    std::unordered_map<std::string, Profile> profiles;
    
    // Создание контроллера по типу
    std::shared_ptr<Controller> create_controller(ControllerType type);
    
    // Инициализация SDL
    void init_sdl();
    void cleanup_sdl();
};

} // namespace md

#endif // INPUT_HPP
