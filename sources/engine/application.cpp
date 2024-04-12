#include "application.h"
#include <glad/glad.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_sdl.h>
#include <SDL2/SDL.h>

extern void game_init();
extern void game_update();
extern void game_render();
extern void imgui_render();
extern void start_time();
extern void update_time();
extern void close_game();

typedef void *SDL_GLContext;

struct SDLContext
{
  SDL_Window *window = nullptr;
  SDL_GLContext gl_context = nullptr;
};

SDLContext context;

void init_application(const char *project_name, int width, int height, bool full_screen)
{
  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

  size_t window_flags = SDL_WINDOW_OPENGL;
  if (full_screen)
    window_flags |= SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE;
  context.window = SDL_CreateWindow(project_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, (SDL_WindowFlags)(window_flags));
  context.gl_context = SDL_GL_CreateContext(context.window);
  SDL_GL_MakeCurrent(context.window, context.gl_context);
  SDL_GL_SetSwapInterval(0);

  if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
  {
    throw std::runtime_error{"Glad error"};
  }
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForOpenGL(context.window, context.gl_context);
  const char *glsl_version = "#version 450";
  ImGui_ImplOpenGL3_Init(glsl_version);
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_MULTISAMPLE);
}

void close_application()
{
  close_game();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  SDL_Quit();
}


static bool sdl_event_handler()
{
  SDL_Event event;
  bool running = true;
  while (SDL_PollEvent(&event))
  {
    ImGui_ImplSDL2_ProcessEvent(&event);
    switch(event.type){
      case SDL_QUIT: running = false; break;

      case SDL_KEYDOWN:

      case SDL_KEYUP: input.event_process(event.key);

      if(event.key.keysym.sym == SDLK_ESCAPE)
        running = false;


      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP: input.event_process(event.button); break;

      case SDL_MOUSEMOTION: input.event_process(event.motion); break;

      case SDL_MOUSEWHEEL: input.event_process(event.wheel); break;

      case SDL_WINDOWEVENT: break;
    }
  }
  return running;
}

void main_loop()
{
  start_time();
  game_init();

  bool running = true;
  while (running)
  {
    update_time();

		running = sdl_event_handler();

    if (running)
    {
      game_update();
      SDL_GL_SwapWindow(context.window);

      game_render();

      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplSDL2_NewFrame(context.window);
      ImGui::NewFrame();
      {
        if (ImGui::BeginMainMenuBar())
        {
          ImGui::EndMainMenuBar();
        }
      }
      imgui_render();

      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
	}
}


float get_aspect_ratio()
{
  int width, height;
  SDL_GL_GetDrawableSize(context.window, &width, &height);
  return (float)width / height;
}