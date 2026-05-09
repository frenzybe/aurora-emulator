#ifndef INPUT_CONTROLLER_HPP
#define INPUT_CONTROLLER_HPP

#include "types.hpp"
#include <cstdint>

namespace md {

// Типы контроллеров
enum ControllerType {
    CTRL_NONE = 0,
    CTRL_MD_3BUTTON,
    CTRL_MD_6BUTTON,
    CTRL_MOUSE,
    CTRL_LIGHTGUN,
    CTRL_DUALSENSE,
    CTRL_XBOX,
    CTRL_SWITCH_PRO
};

// Кнопки Mega Drive
enum Button {
    BTN_UP = 0,
    BTN_DOWN,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_A,      // B на EU/JP
    BTN_B,      // A на EU/JP
    BTN_C,
    BTN_X,
    BTN_Y,
    BTN_Z,
    BTN_START,
    BTN_MODE,   // не используется в MD
    BTN_MAX
};

// Аналоговые оси (для современных контроллеров)
enum Axis {
    AXIS_LEFT_X,
    AXIS_LEFT_Y,
    AXIS_RIGHT_X,
    AXIS_RIGHT_Y,
    AXIS_L2,     // триггеры
    AXIS_R2,
    AXIS_MAX
};

// Состояние кнопки
struct ButtonState {
    bool pressed = false;
    bool just_pressed = false;  // только что нажата
    bool just_released = false; // только что отпущена
};

// Состояние контроллера
struct ControllerState {
    std::array<ButtonState, BTN_MAX> buttons;
    std::array<float, AXIS_MAX> axes;  // -1.0 до 1.0
};

// Абстрактный контроллер
class Controller {
public:
    virtual ~Controller() = default;
    
    virtual void update() = 0;  // Считать состояние с устройства
    virtual ControllerType get_type() const = 0;
    
    const ControllerState& get_state() const { return state; }
    
    // Маппинг на кнопки Mega Drive
    virtual ButtonState get_md_button(Button btn) const = 0;
    
    // Вибрация
    virtual void set_rumble(float /*left*/, float /*right*/) {}  // 0.0-1.0

protected:
    ControllerState state;
};

// Простой контроллер Mega Drive (3-button)
// В будущем: расширить до 6-button, мыши, светового пистолета
class MDController : public Controller {
public:
    MDController() = default;
    void update() override;
    ControllerType get_type() const override { return CTRL_MD_3BUTTON; }
    ButtonState get_md_button(Button btn) const override;
    void set_button(Button btn, bool pressed);  // Для тестов/внутреннего использования
    void set_rumble(float /*left*/, float /*right*/) override {}  // Заглушка
};

} // namespace md

#endif // INPUT_CONTROLLER_HPP
