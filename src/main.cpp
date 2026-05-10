#include "emulator_core.hpp"
#include "ra_integration.hpp"
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Global Application State
struct {
  bool running = true;
  std::string shader_mode = "none";
  int volume = 80;
  std::string core_path;
  std::string ra_user;
  std::string ra_token;
  std::string state_path;
  std::string states_dir;
  struct {
    std::string input_mode = "keyboard";
    int up = SDL_SCANCODE_UP;
    int down = SDL_SCANCODE_DOWN;
    int left = SDL_SCANCODE_LEFT;
    int right = SDL_SCANCODE_RIGHT;
    int a = SDL_SCANCODE_Z;
    int b = SDL_SCANCODE_X;
    int c = SDL_SCANCODE_C;
    int x = SDL_SCANCODE_A;
    int y = SDL_SCANCODE_S;
    int l = SDL_SCANCODE_Q;
    int r = SDL_SCANCODE_W;
    int select = SDL_SCANCODE_RSHIFT;
    int start = SDL_SCANCODE_RETURN;
    struct {
      int up = SDL_CONTROLLER_BUTTON_DPAD_UP;
      int down = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
      int left = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
      int right = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
      int a = SDL_CONTROLLER_BUTTON_A;
      int b = SDL_CONTROLLER_BUTTON_B;
      int c = SDL_CONTROLLER_BUTTON_X;
      int x = SDL_CONTROLLER_BUTTON_Y;
      int y = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
      int l = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
      int r = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
      int l2 = SDL_CONTROLLER_BUTTON_INVALID;
      int r2 = SDL_CONTROLLER_BUTTON_INVALID;
      int l3 = SDL_CONTROLLER_BUTTON_INVALID;
      int r3 = SDL_CONTROLLER_BUTTON_INVALID;
      int select = SDL_CONTROLLER_BUTTON_BACK;
      int start = SDL_CONTROLLER_BUTTON_START;
    } gp;
  } p1, p2;
} app_state;

SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;
SDL_Texture *game_texture = nullptr;
EmulatorCore core;
RA_Manager *ra_manager = nullptr;
TTF_Font *osd_font = nullptr;
std::string osd_text;
uint32_t osd_timer = 0;
std::string screenshot_request_path = "";

static SDL_GameController *controllers[2] = {nullptr, nullptr};
static SDL_Joystick *joysticks[2] = {nullptr, nullptr};

static void show_notification(const std::string &text) {
  osd_text = text;
  osd_timer = 180; // 3 seconds at 60fps
}

static void close_controllers() {
  for (int i = 0; i < 2; i++) {
    if (controllers[i]) {
      SDL_GameControllerClose(controllers[i]);
      controllers[i] = nullptr;
    }
    if (joysticks[i]) {
      SDL_JoystickClose(joysticks[i]);
      joysticks[i] = nullptr;
    }
  }
}

static int16_t get_input(int port, unsigned id) {
  if (port > 1)
    return 0;
  auto &p = (port == 0) ? app_state.p1 : app_state.p2;

  if (p.input_mode == "keyboard") {
    const uint8_t *keys = SDL_GetKeyboardState(NULL);
    int16_t res = 0;
    switch (id) {
    case RETRO_DEVICE_ID_JOYPAD_UP: res = keys[p.up]; break;
    case RETRO_DEVICE_ID_JOYPAD_DOWN: res = keys[p.down]; break;
    case RETRO_DEVICE_ID_JOYPAD_LEFT: res = keys[p.left]; break;
    case RETRO_DEVICE_ID_JOYPAD_RIGHT: res = keys[p.right]; break;
    case RETRO_DEVICE_ID_JOYPAD_A: res = keys[p.a]; break;
    case RETRO_DEVICE_ID_JOYPAD_B: res = keys[p.b]; break;
    case RETRO_DEVICE_ID_JOYPAD_X: res = keys[p.x]; break;
    case RETRO_DEVICE_ID_JOYPAD_Y: res = keys[p.y]; break;
    case RETRO_DEVICE_ID_JOYPAD_L: res = keys[p.l]; break;
    case RETRO_DEVICE_ID_JOYPAD_R: res = keys[p.r]; break;
    case RETRO_DEVICE_ID_JOYPAD_SELECT: res = keys[p.select]; break;
    case RETRO_DEVICE_ID_JOYPAD_START: res = keys[p.start]; break;
    default: res = 0; break;
    }
    return res ? 32767 : 0;
  } else {
    // Input polling and device management
    if (SDL_NumJoysticks() > 0) {
      if (!controllers[port] && !joysticks[port]) {
        static uint32_t last_scan[2] = {0, 0};
        if (SDL_GetTicks() - last_scan[port] > 2000) { // Retry every 2 seconds if not found
          int found_devices = 0;
          for (int i = 0; i < SDL_NumJoysticks(); i++) {
            if (found_devices == port) {
              if (SDL_IsGameController(i)) {
                controllers[port] = SDL_GameControllerOpen(i);
                if (controllers[port]) std::cout << "[Input] P" << (port + 1) << " assigned to Controller: " << SDL_GameControllerName(controllers[port]) << std::endl;
              } else {
                joysticks[port] = SDL_JoystickOpen(i);
                if (joysticks[port]) std::cout << "[Input] P" << (port + 1) << " assigned to raw Joystick: " << SDL_JoystickName(joysticks[port]) << std::endl;
              }
              break;
            }
            found_devices++;
          }
          last_scan[port] = SDL_GetTicks();
        }
      }
      
      SDL_GameController *controller = controllers[port];
      if (controller && SDL_GameControllerGetAttached(controller)) {
        switch (id) {
        case RETRO_DEVICE_ID_JOYPAD_UP:
          return SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)p.gp.up) ||
                 (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) < -16000);
        case RETRO_DEVICE_ID_JOYPAD_DOWN:
          return SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)p.gp.down) ||
                 (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) > 16000);
        case RETRO_DEVICE_ID_JOYPAD_LEFT:
          return SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)p.gp.left) ||
                 (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) < -16000);
        case RETRO_DEVICE_ID_JOYPAD_RIGHT:
          return SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)p.gp.right) ||
                 (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) > 16000);
        case RETRO_DEVICE_ID_JOYPAD_A: return SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)p.gp.a);
        case RETRO_DEVICE_ID_JOYPAD_B: return SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)p.gp.b);
        case RETRO_DEVICE_ID_JOYPAD_X: return SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)p.gp.x);
        case RETRO_DEVICE_ID_JOYPAD_Y: return SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)p.gp.y);
        case RETRO_DEVICE_ID_JOYPAD_L: return SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)p.gp.l);
        case RETRO_DEVICE_ID_JOYPAD_R: return SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)p.gp.r);
        case RETRO_DEVICE_ID_JOYPAD_SELECT: return SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)p.gp.select);
        case RETRO_DEVICE_ID_JOYPAD_START: return SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)p.gp.start);
        }
      } 
      // Handle Raw Joystick (Fallback)
      else if (joysticks[port] && SDL_JoystickGetAttached(joysticks[port])) {
        SDL_Joystick *joy = joysticks[port];
        switch (id) {
          case RETRO_DEVICE_ID_JOYPAD_UP: return SDL_JoystickGetButton(joy, p.gp.up) || (SDL_JoystickGetAxis(joy, 1) < -16000);
          case RETRO_DEVICE_ID_JOYPAD_DOWN: return SDL_JoystickGetButton(joy, p.gp.down) || (SDL_JoystickGetAxis(joy, 1) > 16000);
          case RETRO_DEVICE_ID_JOYPAD_LEFT: return SDL_JoystickGetButton(joy, p.gp.left) || (SDL_JoystickGetAxis(joy, 0) < -16000);
          case RETRO_DEVICE_ID_JOYPAD_RIGHT: return SDL_JoystickGetButton(joy, p.gp.right) || (SDL_JoystickGetAxis(joy, 0) > 16000);
          case RETRO_DEVICE_ID_JOYPAD_A: return SDL_JoystickGetButton(joy, p.gp.a);
          case RETRO_DEVICE_ID_JOYPAD_B: return SDL_JoystickGetButton(joy, p.gp.b);
          case RETRO_DEVICE_ID_JOYPAD_X: return SDL_JoystickGetButton(joy, p.gp.x);
          case RETRO_DEVICE_ID_JOYPAD_Y: return SDL_JoystickGetButton(joy, p.gp.y);
          case RETRO_DEVICE_ID_JOYPAD_SELECT: return SDL_JoystickGetButton(joy, p.gp.select);
          case RETRO_DEVICE_ID_JOYPAD_START: return SDL_JoystickGetButton(joy, p.gp.start);
        }
      }
    }
  }
  return 0;
}

static int16_t input_state_cb(unsigned port, unsigned device, unsigned index,
                               unsigned id) {
  return get_input(port, id);
}

static void video_refresh(const void *data, unsigned w, unsigned h,
                          size_t pitch) {
  if (!data)
    return;

  int fmt = core.get_pixel_format();
  static unsigned last_w = 0, last_h = 0;
  static int last_fmt = -1;
  static bool first_frame_logged = false;

  if (!first_frame_logged) {
    std::cout << "[Player] First frame received: " << w << "x" << h
              << " (Format: " << fmt << ")" << std::endl;
    first_frame_logged = true;
  }

  if (!game_texture || last_w != w || last_h != h || last_fmt != fmt) {
    if (game_texture)
      SDL_DestroyTexture(game_texture);

    Uint32 sdl_fmt = SDL_PIXELFORMAT_RGB565;
    if (fmt == RETRO_PIXEL_FORMAT_XRGB8888)
      sdl_fmt = SDL_PIXELFORMAT_ARGB8888;
    else if (fmt == RETRO_PIXEL_FORMAT_0RGB1555)
      sdl_fmt = SDL_PIXELFORMAT_ARGB1555;

    game_texture =
        SDL_CreateTexture(renderer, sdl_fmt, SDL_TEXTUREACCESS_STREAMING, w, h);
    last_w = w;
    last_h = h;
    last_fmt = fmt;
  }

  void *pixels;
  int m_pitch;
  if (SDL_LockTexture(game_texture, nullptr, &pixels, &m_pitch) == 0) {
    int bpp = (fmt == RETRO_PIXEL_FORMAT_XRGB8888) ? 4 : 2;
    for (unsigned y = 0; y < h; y++)
      memcpy((uint8_t *)pixels + y * m_pitch, (uint8_t *)data + y * pitch,
             w * bpp);
    SDL_UnlockTexture(game_texture);
  }

  if (!screenshot_request_path.empty()) {
    SDL_Surface *sshot = SDL_CreateRGBSurfaceWithFormat(0, w, h, 24, SDL_PIXELFORMAT_RGB24);
    if (sshot) {
      int bpp = (fmt == RETRO_PIXEL_FORMAT_XRGB8888) ? 4 : 2;
      Uint32 sshot_fmt = (fmt == RETRO_PIXEL_FORMAT_XRGB8888) ? SDL_PIXELFORMAT_ARGB8888 : 
                         (fmt == RETRO_PIXEL_FORMAT_0RGB1555 ? SDL_PIXELFORMAT_ARGB1555 : SDL_PIXELFORMAT_RGB565);
      SDL_Surface *temp = SDL_CreateRGBSurfaceWithFormat(0, w, h, bpp * 8, sshot_fmt);
      if (temp) {
        if (SDL_LockSurface(temp) == 0) {
          for (unsigned y = 0; y < h; y++) {
            uint8_t *dst = (uint8_t *)temp->pixels + y * temp->pitch;
            uint8_t *src = (uint8_t *)data + y * pitch;
            memcpy(dst, src, w * bpp);
            if (bpp == 4) {
              uint32_t *p = (uint32_t *)dst;
              for (unsigned x = 0; x < w; x++) p[x] |= 0xFF000000;
            }
          }
          SDL_UnlockSurface(temp);
        }
        SDL_BlitSurface(temp, NULL, sshot, NULL);
        SDL_FreeSurface(temp);
        IMG_SavePNG(sshot, screenshot_request_path.c_str());
        std::cout << "[Core] Screenshot saved to: " << screenshot_request_path << std::endl;
      }
      SDL_FreeSurface(sshot);
    }
    screenshot_request_path.clear();
  }
}

static size_t audio_refresh(const int16_t *data, size_t frames) {
  if (!data || frames == 0)
    return 0;
  while (SDL_GetQueuedAudioSize(1) > 8192)
    SDL_Delay(1);

  std::vector<int16_t> buffered(frames * 2);
  float vol_factor = app_state.volume / 100.0f;
  for (size_t i = 0; i < frames * 2; i++)
    buffered[i] = (int16_t)(data[i] * vol_factor);
  SDL_QueueAudio(1, buffered.data(), (Uint32)(frames * 2 * sizeof(int16_t)));
  return frames;
}

int main(int argc, char *argv[]) {
  std::string rom_path;
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    
    if (arg.find("--") == 0) {
      // It's a flag, handle it and skip its value if it has one
      if (i + 1 < argc) {
        std::string val = argv[++i];
        if (arg == "--shader") app_state.shader_mode = val;
        else if (arg == "--volume") app_state.volume = std::clamp(std::stoi(val), 0, 100);
        else if (arg == "--core") app_state.core_path = val;
        else if (arg == "--states-dir") app_state.states_dir = val;
        else if (arg == "--state") app_state.state_path = val;
        else if (arg == "--ra-user") app_state.ra_user = val;
        else if (arg == "--ra-token") app_state.ra_token = val;
        else if (arg == "--p1-input") app_state.p1.input_mode = val;
        else if (arg == "--p1-key-up") app_state.p1.up = std::stoi(val);
        else if (arg == "--p1-key-down") app_state.p1.down = std::stoi(val);
        else if (arg == "--p1-key-left") app_state.p1.left = std::stoi(val);
        else if (arg == "--p1-key-right") app_state.p1.right = std::stoi(val);
        else if (arg == "--p1-key-a") app_state.p1.a = std::stoi(val);
        else if (arg == "--p1-key-b") app_state.p1.b = std::stoi(val);
        else if (arg == "--p1-key-select") app_state.p1.select = std::stoi(val);
        else if (arg == "--p1-key-start") app_state.p1.start = std::stoi(val);
        else if (arg == "--p1-gp-up") app_state.p1.gp.up = std::stoi(val);
        else if (arg == "--p1-gp-down") app_state.p1.gp.down = std::stoi(val);
        else if (arg == "--p1-gp-left") app_state.p1.gp.left = std::stoi(val);
        else if (arg == "--p1-gp-right") app_state.p1.gp.right = std::stoi(val);
        else if (arg == "--p1-gp-a") app_state.p1.gp.a = std::stoi(val);
        else if (arg == "--p1-gp-b") app_state.p1.gp.b = std::stoi(val);
        else if (arg == "--p1-gp-x") app_state.p1.gp.x = std::stoi(val);
        else if (arg == "--p1-gp-y") app_state.p1.gp.y = std::stoi(val);
        else if (arg == "--p1-gp-l") app_state.p1.gp.l = std::stoi(val);
        else if (arg == "--p1-gp-r") app_state.p1.gp.r = std::stoi(val);
        else if (arg == "--p1-gp-select") app_state.p1.gp.select = std::stoi(val);
        else if (arg == "--p1-gp-start") app_state.p1.gp.start = std::stoi(val);
        else if (arg == "--p2-input") app_state.p2.input_mode = val;
        else if (arg == "--p2-key-up") app_state.p2.up = std::stoi(val);
        else if (arg == "--p2-key-down") app_state.p2.down = std::stoi(val);
        else if (arg == "--p2-key-left") app_state.p2.left = std::stoi(val);
        else if (arg == "--p2-key-right") app_state.p2.right = std::stoi(val);
        else if (arg == "--p2-key-a") app_state.p2.a = std::stoi(val);
        else if (arg == "--p2-key-b") app_state.p2.b = std::stoi(val);
        else if (arg == "--p2-key-select") app_state.p2.select = std::stoi(val);
        else if (arg == "--p2-key-start") app_state.p2.start = std::stoi(val);
        else if (arg == "--p2-gp-up") app_state.p2.gp.up = std::stoi(val);
        else if (arg == "--p2-gp-down") app_state.p2.gp.down = std::stoi(val);
        else if (arg == "--p2-gp-left") app_state.p2.gp.left = std::stoi(val);
        else if (arg == "--p2-gp-right") app_state.p2.gp.right = std::stoi(val);
        else if (arg == "--p2-gp-a") app_state.p2.gp.a = std::stoi(val);
        else if (arg == "--p2-gp-b") app_state.p2.gp.b = std::stoi(val);
        else if (arg == "--p2-gp-select") app_state.p2.gp.select = std::stoi(val);
        else if (arg == "--p2-gp-start") app_state.p2.gp.start = std::stoi(val);
      }
    } else {
      // It's not a flag, so it must be the ROM path
      rom_path = arg;
    }
  }

  if (rom_path.empty()) {
    std::cerr << "[Core] Error: No ROM path specified!" << std::endl;
    return 1;
  }

  std::cout << "[Core] Target ROM: " << rom_path << std::endl;
  std::cout << "[Core] Target Core: " << app_state.core_path << std::endl;

  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
  SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "1");
  SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4, "1");
  SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "1");
  
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) {
      std::cerr << "[Core] SDL_Init failed: " << SDL_GetError() << std::endl;
      return 1;
  }
  TTF_Init();
  IMG_Init(IMG_INIT_PNG);

  std::cout << "[Player] Scanning for input devices..." << std::endl;
  for (int i = 0; i < SDL_NumJoysticks(); i++) {
      const char* name = SDL_JoystickNameForIndex(i);
      std::cout << "  - Device " << i << ": " << (name ? name : "Unknown") 
                << " [Controller: " << (SDL_IsGameController(i) ? "YES" : "NO") << "]" << std::endl;
  }

  window = SDL_CreateWindow("Aurora Player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 960, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!window) {
      std::cerr << "[Core] Window creation failed: " << SDL_GetError() << std::endl;
      return 1;
  }
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
      std::cerr << "[Core] Renderer creation failed: " << SDL_GetError() << std::endl;
      return 1;
  }

  core.set_video_cb(video_refresh);
  core.set_audio_cb(audio_refresh);
  core.set_input_cb(input_state_cb);

  std::cout << "[Core] Loading core DLL..." << std::endl;
  if (!core.load_core(app_state.core_path)) {
      std::cerr << "[Core] Failed to load core DLL!" << std::endl;
      return 1;
  }
  std::cout << "[Core] Core DLL loaded successfully." << std::endl;

  std::cout << "[Core] Loading game ROM..." << std::endl;
  if (!core.load_game(rom_path)) {
      std::cerr << "[Core] Failed to load game ROM: " << rom_path << std::endl;
      return 1;
  }
  std::cout << "[Core] Game loaded successfully." << std::endl;

  if (!app_state.state_path.empty()) {
    if (core.load_state(app_state.state_path)) {
      std::cout << "[Core] Successfully loaded initial state: " << app_state.state_path << std::endl;
      show_notification("STATE LOADED");
    } else {
      std::cerr << "[Core] Failed to load initial state: " << app_state.state_path << std::endl;
    }
  }

  if (!app_state.ra_user.empty() && !app_state.ra_token.empty()) {
    ra_manager = new RA_Manager();
    ra_manager->init(app_state.ra_user, app_state.ra_token);
    ra_manager->load_game(rom_path, &core);
  }

  auto &av = core.get_av_info();
  SDL_AudioSpec spec = {(int)av.sample_rate, AUDIO_S16SYS, 2, 0, 512, 0, 0, NULL, NULL};
  SDL_OpenAudio(&spec, NULL);
  SDL_PauseAudio(0);

  while (app_state.running) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_QUIT) app_state.running = false;
      if (ev.type == SDL_KEYDOWN && ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE) app_state.running = false;
    }

    static bool save_pressed = false;
    static bool load_pressed = false;

    // Process system commands and input
    for (int p_idx = 0; p_idx < 2; p_idx++) {
        get_input(p_idx, 0); // Trigger auto-open
        
        bool select = false, start = false, l1 = false, r1 = false;
        if (controllers[p_idx]) {
            select = SDL_GameControllerGetButton(controllers[p_idx], SDL_CONTROLLER_BUTTON_BACK);
            start = SDL_GameControllerGetButton(controllers[p_idx], SDL_CONTROLLER_BUTTON_START);
            l1 = SDL_GameControllerGetButton(controllers[p_idx], SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
            r1 = SDL_GameControllerGetButton(controllers[p_idx], SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
        } else if (joysticks[p_idx]) {
            auto &p = (p_idx == 0) ? app_state.p1 : app_state.p2;
            select = SDL_JoystickGetButton(joysticks[p_idx], p.gp.select);
            start = SDL_JoystickGetButton(joysticks[p_idx], p.gp.start);
            l1 = SDL_JoystickGetButton(joysticks[p_idx], 4); 
            r1 = SDL_JoystickGetButton(joysticks[p_idx], 5);
        }

        if (select && start) app_state.running = false;
        
        if (select && r1) {
            if (!save_pressed && !app_state.states_dir.empty()) {
                std::string timestamp = std::to_string(time(nullptr));
                std::string path = app_state.states_dir + "/Slot_" + timestamp + ".state";
                if (core.save_state(path)) {
                    screenshot_request_path = app_state.states_dir + "/Slot_" + timestamp + ".png";
                    app_state.state_path = path; // UPDATE CURRENT STATE PATH FOR INSTANT ROLLBACK
                    show_notification("STATE SAVED");
                }
                save_pressed = true;
            }
        } else if (select && l1) {
            if (!load_pressed && !app_state.state_path.empty()) {
                if (core.load_state(app_state.state_path)) {
                    show_notification("STATE LOADED");
                }
                load_pressed = true;
            }
        } 
        
        // Reset press flags when buttons are released
        if (!(select && r1)) save_pressed = false;
        if (!(select && l1)) load_pressed = false;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    core.run();
    if (ra_manager) ra_manager->update();

    if (game_texture) {
      int win_w, win_h;
      SDL_GetWindowSize(window, &win_w, &win_h);
      float game_aspect = (float)av.width / av.height;
      SDL_Rect dest;
      if ((float)win_w/win_h > game_aspect) {
        dest.h = win_h; dest.w = (int)(win_h * game_aspect); dest.x = (win_w - dest.w) / 2; dest.y = 0;
      } else {
        dest.w = win_w; dest.h = (int)(win_w / game_aspect); dest.x = 0; dest.y = (win_h - dest.h) / 2;
      }
      SDL_RenderCopy(renderer, game_texture, nullptr, &dest);
    }

    if (ra_manager) ra_manager->render(renderer);
    if (osd_timer > 0) {
        if (!osd_font) {
            // Try to load a system font
            const char* font_paths[] = {
                "C:\\Windows\\Fonts\\arial.ttf",
                "/Library/Fonts/Arial.ttf",
                "/System/Library/Fonts/Supplemental/Arial.ttf",
                "/System/Library/Fonts/Helvetica.ttc",
                "/System/Library/Fonts/Geneva.ttf",
                "/System/Library/Fonts/Menlo.ttc",
                "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
            };
            for (const char* p : font_paths) {
                osd_font = TTF_OpenFont(p, 24);
                if (osd_font) {
                    std::cout << "[Core] OSD Font loaded: " << p << std::endl;
                    break;
                }
            }
            if (!osd_font) {
                std::cerr << "[Core] Warning: Failed to load any system font for OSD!" << std::endl;
                osd_timer = 0; // Don't try again
            }
        }

        if (osd_font) {
            SDL_Color white = {255, 255, 255, 255};
            SDL_Surface *surf = TTF_RenderText_Blended(osd_font, osd_text.c_str(), white);
            if (surf) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                if (tex) {
                    SDL_Rect rect = {20, 20, surf->w, surf->h};
                    // Enable blending for the shadow/background
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
                    SDL_Rect bg = {rect.x - 5, rect.y - 5, rect.w + 10, rect.h + 10};
                    SDL_RenderFillRect(renderer, &bg);
                    SDL_RenderCopy(renderer, tex, nullptr, &rect);
                    SDL_DestroyTexture(tex);
                }
                SDL_FreeSurface(surf);
            }
        }
        osd_timer--;
    }
    SDL_RenderPresent(renderer);
  }

  core.reset();
  close_controllers();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  return 0;
}
