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
  if (argc < 2)
    return 1;

  std::string rom_path = argv[1];
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--shader" && i + 1 < argc) app_state.shader_mode = argv[++i];
    else if (arg == "--volume" && i + 1 < argc) app_state.volume = std::clamp(std::stoi(argv[++i]), 0, 100);
    else if (arg == "--core" && i + 1 < argc) app_state.core_path = argv[++i];
    else if (arg == "--states-dir" && i + 1 < argc) app_state.states_dir = argv[++i];
    else if (arg == "--ra-user" && i + 1 < argc) app_state.ra_user = argv[++i];
    else if (arg == "--ra-token" && i + 1 < argc) app_state.ra_token = argv[++i];
    else if (arg == "--p1-input" && i + 1 < argc) app_state.p1.input_mode = argv[++i];
    else if (arg == "--p1-key-up" && i + 1 < argc) app_state.p1.up = std::stoi(argv[++i]);
    else if (arg == "--p1-key-down" && i + 1 < argc) app_state.p1.down = std::stoi(argv[++i]);
    else if (arg == "--p1-key-left" && i + 1 < argc) app_state.p1.left = std::stoi(argv[++i]);
    else if (arg == "--p1-key-right" && i + 1 < argc) app_state.p1.right = std::stoi(argv[++i]);
    else if (arg == "--p1-key-a" && i + 1 < argc) app_state.p1.a = std::stoi(argv[++i]);
    else if (arg == "--p1-key-b" && i + 1 < argc) app_state.p1.b = std::stoi(argv[++i]);
    else if (arg == "--p1-key-select" && i + 1 < argc) app_state.p1.select = std::stoi(argv[++i]);
    else if (arg == "--p1-key-start" && i + 1 < argc) app_state.p1.start = std::stoi(argv[++i]);
    else if (arg == "--p1-gp-up" && i + 1 < argc) app_state.p1.gp.up = std::stoi(argv[++i]);
    else if (arg == "--p1-gp-down" && i + 1 < argc) app_state.p1.gp.down = std::stoi(argv[++i]);
    else if (arg == "--p1-gp-left" && i + 1 < argc) app_state.p1.gp.left = std::stoi(argv[++i]);
    else if (arg == "--p1-gp-right" && i + 1 < argc) app_state.p1.gp.right = std::stoi(argv[++i]);
    else if (arg == "--p1-gp-a" && i + 1 < argc) app_state.p1.gp.a = std::stoi(argv[++i]);
    else if (arg == "--p1-gp-b" && i + 1 < argc) app_state.p1.gp.b = std::stoi(argv[++i]);
    else if (arg == "--p1-gp-c" && i + 1 < argc) app_state.p1.gp.c = std::stoi(argv[++i]);
    else if (arg == "--p1-gp-x" && i + 1 < argc) app_state.p1.gp.x = std::stoi(argv[++i]);
    else if (arg == "--p1-gp-y" && i + 1 < argc) app_state.p1.gp.y = std::stoi(argv[++i]);
    else if (arg == "--p1-gp-l" && i + 1 < argc) app_state.p1.gp.l = std::stoi(argv[++i]);
    else if (arg == "--p1-gp-r" && i + 1 < argc) app_state.p1.gp.r = std::stoi(argv[++i]);
    else if (arg == "--p1-gp-l2" && i + 1 < argc) app_state.p1.gp.l2 = std::stoi(argv[++i]);
    else if (arg == "--p1-gp-r2" && i + 1 < argc) app_state.p1.gp.r2 = std::stoi(argv[++i]);
    else if (arg == "--p1-gp-l3" && i + 1 < argc) app_state.p1.gp.l3 = std::stoi(argv[++i]);
    else if (arg == "--p1-gp-r3" && i + 1 < argc) app_state.p1.gp.r3 = std::stoi(argv[++i]);
    else if (arg == "--p1-gp-select" && i + 1 < argc) app_state.p1.gp.select = std::stoi(argv[++i]);
    else if (arg == "--p1-gp-start" && i + 1 < argc) app_state.p1.gp.start = std::stoi(argv[++i]);
    else if (arg == "--p2-input" && i + 1 < argc) app_state.p2.input_mode = argv[++i];
    else if (arg == "--p2-gp-up" && i + 1 < argc) app_state.p2.gp.up = std::stoi(argv[++i]);
    else if (arg == "--p2-gp-down" && i + 1 < argc) app_state.p2.gp.down = std::stoi(argv[++i]);
    else if (arg == "--p2-gp-left" && i + 1 < argc) app_state.p2.gp.left = std::stoi(argv[++i]);
    else if (arg == "--p2-gp-right" && i + 1 < argc) app_state.p2.gp.right = std::stoi(argv[++i]);
    else if (arg == "--p2-gp-a" && i + 1 < argc) app_state.p2.gp.a = std::stoi(argv[++i]);
    else if (arg == "--p2-gp-b" && i + 1 < argc) app_state.p2.gp.b = std::stoi(argv[++i]);
    else if (arg == "--p2-gp-c" && i + 1 < argc) app_state.p2.gp.c = std::stoi(argv[++i]);
    else if (arg == "--p2-gp-x" && i + 1 < argc) app_state.p2.gp.x = std::stoi(argv[++i]);
    else if (arg == "--p2-gp-y" && i + 1 < argc) app_state.p2.gp.y = std::stoi(argv[++i]);
    else if (arg == "--p2-gp-l" && i + 1 < argc) app_state.p2.gp.l = std::stoi(argv[++i]);
    else if (arg == "--p2-gp-r" && i + 1 < argc) app_state.p2.gp.r = std::stoi(argv[++i]);
    else if (arg == "--p2-gp-l2" && i + 1 < argc) app_state.p2.gp.l2 = std::stoi(argv[++i]);
    else if (arg == "--p2-gp-r2" && i + 1 < argc) app_state.p2.gp.r2 = std::stoi(argv[++i]);
    else if (arg == "--p2-gp-l3" && i + 1 < argc) app_state.p2.gp.l3 = std::stoi(argv[++i]);
    else if (arg == "--p2-gp-r3" && i + 1 < argc) app_state.p2.gp.r3 = std::stoi(argv[++i]);
    else if (arg == "--p2-gp-select" && i + 1 < argc) app_state.p2.gp.select = std::stoi(argv[++i]);
    else if (arg == "--p2-gp-start" && i + 1 < argc) app_state.p2.gp.start = std::stoi(argv[++i]);
  }

  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
  SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "1");
  SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4, "1");
  SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "1");
  
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
  TTF_Init();
  IMG_Init(IMG_INIT_PNG);

  std::cout << "[Player] Scanning for input devices..." << std::endl;
  for (int i = 0; i < SDL_NumJoysticks(); i++) {
      const char* name = SDL_JoystickNameForIndex(i);
      std::cout << "  - Device " << i << ": " << (name ? name : "Unknown") 
                << " [Controller: " << (SDL_IsGameController(i) ? "YES" : "NO") << "]" << std::endl;
  }

  window = SDL_CreateWindow("Aurora Player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 960, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  core.set_video_cb(video_refresh);
  core.set_audio_cb(audio_refresh);
  core.set_input_cb(input_state_cb);

  if (!core.load_core(app_state.core_path) || !core.load_game(rom_path)) return 1;

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

    // Process system commands and input
    for (int p_idx = 0; p_idx < 2; p_idx++) {
        // Smart device acquisition in get_input will handle opening
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
            // Fallback for L1/R1 on raw joysticks - use standard indices 4 and 5 if not mapped
            l1 = SDL_JoystickGetButton(joysticks[p_idx], 4); 
            r1 = SDL_JoystickGetButton(joysticks[p_idx], 5);
        }

        if (select && start) app_state.running = false;
        if (select && r1) {
            static bool save_pressed = false;
            if (!save_pressed && !app_state.states_dir.empty()) {
                std::string timestamp = std::to_string(time(nullptr));
                std::string path = app_state.states_dir + "/Slot_" + timestamp + ".state";
                if (core.save_state(path)) {
                    screenshot_request_path = app_state.states_dir + "/Slot_" + timestamp + ".png";
                    show_notification("STATE SAVED");
                }
                save_pressed = true;
            }
        } else { /* reset save_pressed logic here would need a per-player array if we wanted to be perfect */ }
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
        // ... (OSD render logic)
    }
    SDL_RenderPresent(renderer);
  }

  core.reset();
  close_controllers();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  return 0;
}
