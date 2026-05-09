#ifndef RENDERER_SOFTWARE_HPP
#define RENDERER_SOFTWARE_HPP

#include "renderer.hpp"
#include <SDL2/SDL.h>

namespace md {

// Software рендерер (CPU-based)
class SoftwareRenderer : public Renderer {
public:
    SoftwareRenderer(VDP* vdp);
    ~SoftwareRenderer();

    void init(int width, int height) override;
    void resize(int width, int height) override;
    void present() override;
    
    void clear(u8 r = 0, u8 g = 0, u8 b = 0) override;
    void draw_pixel(int x, int y, u32 color) override;
    void draw_rect(int x, int y, int w, int h, u32 color) override;
    
    std::vector<u32>& get_framebuffer() override { return framebuffer; }

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* sdl_renderer = nullptr;
    SDL_Texture* texture = nullptr;
    
    std::vector<u32> framebuffer;  // 32-bit RGBA
    std::vector<u32> palette;      // 512 цветов (9-bit → 32-bit)
    
    // Конвертация цвета из VDP CRAM (9-bit) в 32-bit RGBA
    u32 convert_color(u16 cram_color);
    
    // Рендеринг одной сканирующей линии из VDP
    void render_scanline(int line);
    
    // Масштабирование (nearest neighbor)
    void scale_and_present();
    void cleanup();
};

} // namespace md

#endif // RENDERER_SOFTWARE_HPP
