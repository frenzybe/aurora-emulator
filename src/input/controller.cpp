#include "../include/input/controller.hpp"

namespace md {

// Реализация MDController

void MDController::update() {
    // Обновление состояния кнопок
    // В будущем: читать из SDL/ hidapi
}

ButtonState MDController::get_md_button(Button btn) const {
    return state.buttons[static_cast<size_t>(btn)];
}

// Для тестов/внутреннего использования
void MDController::set_button(Button btn, bool pressed) {
    state.buttons[static_cast<size_t>(btn)].pressed = pressed;
}

} // namespace md
