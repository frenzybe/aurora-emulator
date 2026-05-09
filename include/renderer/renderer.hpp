#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "types.hpp"
#include <vector>
#include <memory>

namespace md {

class VDP;

// Абстрактный рендерер
class Renderer {
public:
    Renderer(VDP* vdp);
    virtual ~Renderer() = default;
    
    virtual void init(int width, int height) = 0;
    virtual void resize(int width, int height) = 0;
    virtual void present() = 0;  // Отобразить кадр
    
    // Очистка экрана
    virtual void clear(u8 r = 0, u8 g = 0, u8 b = 0) = 0;
    
    // Рисование пикселя/прямоугольника
    virtual void draw_pixel(int x, int y, u32 color) = 0;
    virtual void draw_rect(int x, int y, int w, int h, u32 color) = 0;
    
    // Получить/установить буфер кадра
    virtual std::vector<u32>& get_framebuffer() = 0;
    
    // Настройки
    void set_scale(float s) { scale = s; }
    void set_filter(const std::string& f) { filter = f; }
    void set_shader(const std::string& s) { shader = s; }

protected:
    VDP* vdp;
    int screen_width = SCREEN_WIDTH;
    int screen_height = SCREEN_HEIGHT_NTSC;
    float scale = 2.0f;
    std::string filter;
    std::string shader;
};

} // namespace md

#endif // RENDERER_HPP
