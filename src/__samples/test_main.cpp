void OutputDebugStringF(const char* fmt, ...);

#include "core/core_include.h"
#include "core/core_include.cpp"

#include "os/win32.h"
#include "os/win32.cpp"

#include "render/render.h"
#include "render/render.cpp"

#include "draw/draw.h"
#include "draw/draw.cpp"

#include "font_provider/font_provider.h"
#include "font_provider/font_provider.cpp"

#include "ui/ui_core.h"
#include "ui/ui_core.cpp"

#include "ui/widgets/ui_widgets.h"
#include "ui/widgets/ui_widgets.cpp"

void paint(R_Handle* window_frame_buffer)
{
  r_prepare_canvas(window_frame_buffer);
  d_begin_batching(*window_frame_buffer);

  r_clear_handle(*window_frame_buffer, black());
  Rect rect = {};
  rect.x      = os_get_client_area_dims().x / 2;
  rect.y      = os_get_client_area_dims().y / 2;
  rect.width  = 50;
  rect.height = 50;
  d_draw_rect(rect, nice_blue());

  d_end_batching();
  r_submit(*window_frame_buffer, d_get_batch_list());
  r_present(*window_frame_buffer, false);
}

int WinMain(HINSTANCE app_instance, HINSTANCE __not_used__, LPSTR cmd, int show)
{
  // Layers we allocate for the runtime 
  allocate_thread_context();
  B32 os_init_succ = os_init();
  // r_init(); 
  // d_init();
  // ui_init();
  // fp_init();

  if (!os_init_succ) { return -1; }

  // TODO: Clear up the window from the os state, the os state should be the shared part and should not have this in there 
  //       The win proc from the os file should then also be removed since it is not general but os specific
  OS_State* win32_state = os_get_state();

  ///////////////////////////////////////////////////////////
  // - Window  
  //
  {
    win32_state->window.window_class.cbSize        = sizeof(WNDCLASSEXA);
    win32_state->window.window_class.style         = CS_HREDRAW|CS_VREDRAW;
    win32_state->window.window_class.lpfnWndProc   = win32_proc;
    win32_state->window.window_class.hInstance     = app_instance;
    win32_state->window.window_class.hIcon         = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(1));
    win32_state->window.window_class.hCursor       = LoadCursorW(0, IDC_ARROW);
    win32_state->window.window_class.hbrBackground = Null;
    win32_state->window.window_class.lpszMenuName  = Null;
    win32_state->window.window_class.lpszClassName = L"pclarity_app_flopper_class_name";
    win32_state->window.window_class.hIconSm       = Null;

    ATOM wc_atom = RegisterClassExW(&win32_state->window.window_class);
    Assert(wc_atom != 0);
    
    win32_state->window.is_transparent = false;
    win32_state->window.handle = CreateWindowExW(
      WS_EX_NOREDIRECTIONBITMAP,
      win32_state->window.window_class.lpszClassName,
      L"PClarity",
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT,
      800, 600,
      Null,
      Null,
      app_instance, 
      Null
    );
    Handle(win32_state->window.handle != 0);
    ShowWindow(win32_state->window.handle, SW_NORMAL);
  }

  ///////////////////////////////////////////////////////////
  // - App loop
  //
  // R_Handle window_frame_buffer_target = r_attach_window(win32_state->window);
  // FP_Font font = fp_load_font(Str8FromC("../data/Roboto.ttf"), 32, rangeU64(0, (U64)u8_max + 1));

  // g_win32_redraw_on_resize_func = paint;
  // g_win32_handle_for_resize_draw = &window_frame_buffer_target;

  for (;!os_window_should_close();)
  {
    F64 frame_start_time_sec = os_get_time_for_timing_sec();
    
    os_frame_begin();
    // paint(&window_frame_buffer_target);
    os_frame_end();

    F64 frame_end_time_sec = os_get_time_for_timing_sec();

    // OutputDebugStringF("Frame time sec: %f\n", frame_end_time_sec - frame_start_time_sec);
    // OutputDebugStringF("FPS:            %f\n", 1.0f/(frame_end_time_sec - frame_start_time_sec));
    // OutputDebugStringF("\n");
  }

  // Not releasing anything since who cares, the system will release all the stuff

  return 0;
}

///////////////////////////////////////////////////////////
// - Main helpers
//
// todo: Code for this is bad
void OutputDebugStringF(const char* fmt, ...)
{
  #if DEBUG_MODE
  va_list argptr;
  va_start(argptr, fmt);
  Scratch scratch = get_scratch(0, 0);
  Data_buffer buffer = data_buffer_make(scratch.arena, 128);
  int ret = vsprintf_s((char*)buffer.data, buffer.count, fmt, argptr);
  if (ret >= 0 && ret < buffer.count)
  {
    OutputDebugStringA((char*)buffer.data);
  } else { InvalidCodePath(); }
  end_scratch(&scratch);
  va_end(argptr);
  #endif
}
              