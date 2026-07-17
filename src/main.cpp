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

#include "pclarity/pclarity.h"
#include "pclarity/pclarity.cpp"

int WinMain(HINSTANCE app_instance, HINSTANCE __not_used__, LPSTR cmd, int show)
{
  // Layers we allocate for the runtime 
  allocate_thread_context();
  B32 os_init_succ = os_init();
  r_init(); 
  d_init();
  ui_init();
  fp_init();

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
  R_Handle window_frame_buffer_target = r_attach_window(win32_state->window);
  FP_Font font = fp_load_font(Str8FromC("../data/Roboto.ttf"), 32, rangeU64(0, (U64)u8_max + 1));

  PCL_State pcl = pcl_init();

  for (;!os_window_should_close();)
  {
    F64 frame_start_time_sec = os_get_time_for_timing_sec();
    
    os_frame_begin();
    r_prepare_canvas(&window_frame_buffer_target);
    d_begin_batching(window_frame_buffer_target);

    /*
    UI_Build(os_get_client_area_dims(), os_get_mouse_pos(), font)
    {
      UI_Col()
      {
        ui_next_width(ui_px(250));
        ui_next_height(ui_px(250));
        ui_next_padded_border(3, nice_green());
        UI_Box* clip_box = ui_box_make(Str8FromC("Clip box"), UI_Box_flag__clip|UI_Box_flag__padded_border);
        
        UI_Parent(clip_box)
        {
          for EachIndex(i, 100)
          {
            ui_text_f("Text %lld", i);
          }
        }

        UI_Box_data box_data = ui_box_data_from_box(clip_box);
        if (box_data.is_found)
        {
          F32 offset = -ui_clip_offset_from_box(clip_box).y;
          OutputDebugStringF("Offset : %f \n", offset);
          B32 is_new_offset = false;
          F32 new_offset = 0.0f;
          pcl_scroll_bar(250, 100, Axis2__x, Str8FromC("Scroll bar"), box_data.rect.height, ui_get_content_dims_from_box(clip_box).y, offset, &new_offset, &is_new_offset);

          if (is_new_offset)
          {
            offset = new_offset;
          }
          ui_box_set_clip_offset_y(clip_box, -offset);
        }
      }



    }
    */

    pcl_frame_update(&pcl);
    pcl_do_ui(font, &pcl);

    r_clear_handle(window_frame_buffer_target, black());
    ui_draw();

    d_end_batching();
    r_submit(window_frame_buffer_target, d_get_batch_list());
    r_present(window_frame_buffer_target, false);
  
    os_frame_end();

    F64 frame_end_time_sec = os_get_time_for_timing_sec();

    OutputDebugStringF("FPS: %.3f, Frame time sec: %.3f\n", 1.0f/(frame_end_time_sec - frame_start_time_sec), (frame_end_time_sec - frame_start_time_sec));
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
              