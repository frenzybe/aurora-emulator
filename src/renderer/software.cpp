#include "renderer/software.hpp"
#include "vdp/vdp.hpp"
#include <SDL2/SDL.h>
#include <algorithm>
#include <iostream>

namespace md {

SoftwareRenderer::SoftwareRenderer(VDP* vdp) : Renderer(vdp) {}

SoftwareRenderer::~SoftwareRenderer() {
    cleanup();
}

void SoftwareRenderer::init(int width, int height) {
    screen_width = width;
    screen_height = height;
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL video init error: " << SDL_GetError() << "\n";
        return;
    }
    
    window = SDL_CreateWindow(
        "Mega Drive Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        static_cast<int>(SCREEN_WIDTH * scale),
        static_cast<int>(SCREEN_HEIGHT_NTSC * scale),
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!window) {
        std::cerr << "Window creation error: " << SDL_GetError() << "\n";
        return;
    }
    
    sdl_renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!sdl_renderer) {
        std::cerr << "Renderer creation error: " << SDL_GetError() << "\n";
        return;
    }
    
    texture = SDL_CreateTexture(
        sdl_renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT_NTSC
    );
    
    framebuffer.resize(SCREEN_WIDTH * SCREEN_HEIGHT_NTSC, 0xFF000000);  // Чёрный
    palette.resize(512);
    
    // Инициализация палитры (512 цветов по умолчанию)
    for (int i = 0; i < 512; i++) {
        palette[i] = 0xFF000000;
    }
}

void SoftwareRenderer::resize(int width, int height) {
    // Изменение размера окна
    SDL_SetWindowSize(window, width, height);
}

void SoftwareRenderer::present() {
    // Рендеринг каждой строки из VDP
    for (u32 line = 0; line < SCREEN_HEIGHT_NTSC; line++) {
        render_scanline(line);
    }
    
    // Копирование в текстуру и отображение
    SDL_UpdateTexture(
        texture,
        nullptr,
        framebuffer.data(),
        SCREEN_WIDTH * sizeof(u32)
    );
    
    SDL_RenderClear(sdl_renderer);
    SDL_RenderCopy(sdl_renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(sdl_renderer);
}

void SoftwareRenderer::clear(u8 r, u8 g, u8 b) {
    u32 color = (0xFF << 24) | (r << 16) | (g << 8) | b;
    std::fill(framebuffer.begin(), framebuffer.end(), color);
}

void SoftwareRenderer::draw_pixel(int x, int y, u32 color) {
    if (x >= 0 && static_cast<u32>(x) < SCREEN_WIDTH && y >= 0 && static_cast<u32>(y) < SCREEN_HEIGHT_NTSC) {
        framebuffer[y * SCREEN_WIDTH + x] = color;
    }
}

void SoftwareRenderer::draw_rect(int x, int y, int w, int h, u32 color) {
    for (int py = y; py < y + h; py++) {
        for (int px = x; px < x + w; px++) {
            draw_pixel(px, py, color);
        }
    }
}

void SoftwareRenderer::render_scanline(int line) {
    // Получение пикселей от VDP
    std::vector<VDP::Pixel> vdp_line;
    vdp->get_scanline(line, vdp_line);
    
    if (vdp_line.empty()) {
        // Заглушка: синий фон
        for (u32 x = 0; x < SCREEN_WIDTH; x++) {
            draw_pixel(x, line, 0xFF0000FF);
        }
        return;
    }
    
    // Копирование с конвертацией цвета
    for (u32 x = 0; x < SCREEN_WIDTH && x < vdp_line.size(); x++) {
        const auto& px = vdp_line[x];
        u32 color = (0xFF << 24) | (px.r << 16) | (px.g << 8) | px.b;
        draw_pixel(x, line, color);
    }
}

void SoftwareRenderer::scale_and_present() {
    // Масштабирование nearest-neighbor
    // Пока что present() уже делает это через SDL
}

void SoftwareRenderer::cleanup() {
    if (texture) SDL_DestroyTexture(texture);
    if (sdl_renderer) SDL_DestroyRenderer(sdl_renderer);
    if (window) SDL_DestroyWindow(window);
}

} // namespace md
