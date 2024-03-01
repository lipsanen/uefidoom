//doomgeneric for cross-platform development library 'Simple DirectMedia Layer'
#include "doomdef.h"
#include "d_event.h"
#include "dlibc.h"
#include "doomkeys.h"
#include "m_argv.h"
#include "doomgeneric.h"

#include <stdio.h>
#include <unistd.h>

#include <stdbool.h>
#include <SDL2/SDL.h>
#include <sys/stat.h>

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* texture;
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

#define KEYQUEUE_SIZE 16

static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

void doomgeneric_Res(uint32_t* width, uint32_t* height) {
  *width = WIDTH;
  *height = HEIGHT;
}

static unsigned char convertToDoomKey(unsigned int key){
  switch (key)
    {
    case SDLK_RETURN:
      key = KEY_ENTER;
      break;
    case SDLK_ESCAPE:
      key = KEY_ESCAPE;
      break;
    case SDLK_LEFT:
      key = KEY_LEFTARROW;
      break;
    case SDLK_RIGHT:
      key = KEY_RIGHTARROW;
      break;
    case SDLK_w:
    case SDLK_UP:
      key = KEY_UPARROW;
      break;
    case SDLK_s:
    case SDLK_DOWN:
      key = KEY_DOWNARROW;
      break;
    case SDLK_LCTRL:
    case SDLK_RCTRL:
      key = KEY_FIRE;
      break;
    case SDLK_SPACE:
      key = KEY_USE;
      break;
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
      key = KEY_RSHIFT;
      break;
    case SDLK_LALT:
    case SDLK_RALT:
      key = KEY_LALT;
      break;
    case SDLK_F2:
      key = KEY_F2;
      break;
    case SDLK_F3:
      key = KEY_F3;
      break;
    case SDLK_F4:
      key = KEY_F4;
      break;
    case SDLK_F5:
      key = KEY_F5;
      break;
    case SDLK_F6:
      key = KEY_F6;
      break;
    case SDLK_F7:
      key = KEY_F7;
      break;
    case SDLK_F8:
      key = KEY_F8;
      break;
    case SDLK_F9:
      key = KEY_F9;
      break;
    case SDLK_F10:
      key = KEY_F10;
      break;
    case SDLK_F11:
      key = KEY_F11;
      break;
    case SDLK_EQUALS:
    case SDLK_PLUS:
      key = KEY_EQUALS;
      break;
    case SDLK_MINUS:
      key = KEY_MINUS;
      break;
    default:
      key = tolower(key);
      break;
    }

  return key;
}

static void handleKeyInput(){
  SDL_Event e;
  while (SDL_PollEvent(&e)){
    if (e.type == SDL_QUIT){
      d_printf("Quit requested");
      atexit(SDL_Quit);
      exit(1);
    }
    if (e.type == SDL_KEYDOWN) {
      DG_AddKeyToQueue(1, convertToDoomKey(e.key.keysym.sym));
    } else if (e.type == SDL_KEYUP) {
      DG_AddKeyToQueue(0, convertToDoomKey(e.key.keysym.sym));
    }
  }
}

static bool mouse_active = false;

void HandleMouse()
{
  bool has_focus = (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;

  if(has_focus != mouse_active) {
    mouse_active = has_focus;
    SDL_SetRelativeMouseMode(has_focus ? SDL_TRUE : SDL_FALSE);
  }

  int x=0, y=0;
  SDL_GetRelativeMouseState(&x, &y);

  if(x != 0) {
    	event_t event;
      d_memset(&event, 0, sizeof(event_t));
      event.type = ev_mouse;
      event.data2 = x;
      D_PostEvent(&event);
  }
}

void DG_DrawFrame()
{
  SDL_UpdateTexture(texture, NULL, DG_ScreenBuffer, WIDTH*sizeof(uint32_t));

  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
  HandleMouse();
  handleKeyInput();
}

int d_putchar(int c) { return putchar(c); }

int main(int argc, char **argv)
{
  doom_data_t doom;
  doomdata_init(&doom);
  window = SDL_CreateWindow("DOOM",
                            SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED,
                            WIDTH,
                            HEIGHT,
                            SDL_WINDOW_SHOWN
                            );

  // Setup renderer
  renderer =  SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED);
  // Clear winow
  SDL_RenderClear( renderer );
  // Render the rect to the screen
  SDL_RenderPresent(renderer);

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, WIDTH, HEIGHT);

  doomgeneric_Create(argc, argv);

  uint64_t startTick = SDL_GetTicks64();

  for (int i = 1; !doom.should_quit; i++)
  {
    uint64_t ticks_needed = (i * 1000) / 35;
    uint64_t current_tick = SDL_GetTicks64();
    while(current_tick < ticks_needed) {
      current_tick = SDL_GetTicks64();
      SDL_Delay(1);
    }

    doomgeneric_Tick(&doom);
  }
  

  return 0;
}
