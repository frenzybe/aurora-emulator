#include "../include/input/dualsense.hpp"
#include <iostream>

// hidapi временно отключен — будет добавлен позже
// #include <hidapi/hidapi.h>

namespace md {


DualSenseController::DualSenseController() = default;

DualSenseController::~DualSenseController() = default;

void DualSenseController::init() {
    // TODO: инициализация через hidapi
    std::cout << "[DualSense] init() — заглушка (hidapi пока не подключен)\n";
}

void DualSenseController::update() {
    // Заглушка: нет hidapi
    // В будущем: читать отчёт от контроллера и парсить
}

void DualSenseController::parse_input_report(const u8* /*data*/, size_t /*len*/) {
    // Заглушка
}

ButtonState DualSenseController::get_md_button(Button btn) const {
    return state.buttons[static_cast<size_t>(btn)];
}

void DualSenseController::set_adaptive_resistance(float /*left*/, float /*right*/) {}
void DualSenseController::set_haptic_effect(const std::vector<float>& /*data*/) {}
void DualSenseController::set_led_color(u8 /*r*/, u8 /*g*/, u8 /*b*/) {}
void DualSenseController::send_output_report() {}

} // namespace md
