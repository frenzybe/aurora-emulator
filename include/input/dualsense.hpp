#ifndef INPUT_DUALSENSE_HPP
#define INPUT_DUALSENSE_HPP

#include "controller.hpp"
#include <cstdint>
#include <vector>
#include <SDL2/SDL_gamecontroller.h>  // Для SDL_CONTROLLER_BUTTON_*

// Для hidapi (будет подключаться в .cpp)
struct hid_device;

namespace md {

// Контроллер DualSense (PlayStation 5)
class DualSenseController : public Controller {
public:
    DualSenseController();
    ~DualSenseController();

    void init();
    void update() override;
    ControllerType get_type() const override { return CTRL_DUALSENSE; }
    
    ButtonState get_md_button(Button btn) const override;
    
    // Управление DualSense фичами
    void set_adaptive_resistance(float left, float right);  // 0.0-1.0
    void set_haptic_effect(const std::vector<float>& data);  // HD haptic
    void set_led_color(u8 r, u8 g, u8 b);
    
    // Проверка доступности
    bool is_connected() const { return connected; }

private:
    [[maybe_unused]] hid_device* dev = nullptr;
    bool connected = false;
    
    // Состояние DualSense
    struct {
        std::array<bool, 16> buttons;
        std::array<float, 6> axes;  // L2/R2 как оси
        u8 touchpad[2] = {0};       // X/Y координаты
        u8 gyro[3] = {0};           // Gyroscope
        u8 accel[3] = {0};          // Accelerometer
    } ds_state;
    
    // Маппинг кнопок DualSense → Mega Drive
    static constexpr int MD_BTN_MAP[BTN_MAX] = {
        /*UP*/     SDL_CONTROLLER_BUTTON_DPAD_UP,
        /*DOWN*/   SDL_CONTROLLER_BUTTON_DPAD_DOWN,
        /*LEFT*/   SDL_CONTROLLER_BUTTON_DPAD_LEFT,
        /*RIGHT*/  SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
        /*A*/      SDL_CONTROLLER_BUTTON_A,
        /*B*/      SDL_CONTROLLER_BUTTON_B,
        /*C*/      SDL_CONTROLLER_BUTTON_X,
        /*X*/      SDL_CONTROLLER_BUTTON_Y,
        /*Y*/      SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
        /*Z*/      SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
        /*START*/  SDL_CONTROLLER_BUTTON_START,
        /*MODE*/   SDL_CONTROLLER_BUTTON_GUIDE
    };
    
    // Отправка отчёта на контроллер
    void send_output_report();
    
    // Парсинг входного отчёта
    void parse_input_report(const u8* data, size_t len);
};

} // namespace md

#endif // INPUT_DUALSENSE_HPP
