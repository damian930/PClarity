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

// struct Framerate_ring_buffer {
//   U32 data_arr[1024];
//   U64 start_index;
//   U64 data_arr_count;
// };

// B32 frame_rate_ring_buffer_add(Framerate_ring_buffer* ring, U64 data, B32 allow_override)
// {
//   B32 data_got_added = false;
//   // todo: do this for frame rate and then have a very light weight way to draw plots for this shit here
  
//   if (ring->data_arr_count < ArrayCount(ring->data_arr) || allow_override)
//   {
//     U64 index_for_new_data = ring->start_index + ring->data_arr_count;
//     if (index_for_new_data >= ArrayCount(ring->data_arr))
//     {
//       index_for_new_data = index_for_new_data % ArrayCount(ring->data_arr); 
//     }
//     if (ring->data_arr_count == ArrayCount(ring->data_arr)) { Assert(index_for_new_data == ring->start_index); }



//   }




// }

int WinMain(HINSTANCE app_instance, HINSTANCE __not_used__, LPSTR cmd, int show)
{
  // Layers we allocate for the runtime 
  profiler_init();
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

  B32 show_debug_stuff = false;

  // os_window_set_full_screen(true);

  // Framerate_ring_buffer framerate_ring_buffer = {};

  U64 frame_counter  = 0;
  U64 prev_frame_fps = 0;

  for (;!os_window_should_close(); frame_counter += 1) 
  {
    profiler_begin_frame();

    ProfBeginGroupF("App frame %lld", frame_counter);

    if (pcl.close_the_app) { break; }

    F64 frame_start_time_sec = os_get_time_for_timing_sec();
    
    os_frame_begin();
    r_prepare_canvas(&window_frame_buffer_target);
    d_begin_batching(window_frame_buffer_target);

    // TODO: Remove this and handle Alt+F4 if it has to be handled manually
    B32 close_the_app = false;
    for (OS_Event* ev = os_get_frame_event_list()->first; ev; ev = ev->next)
    {
      if (ev->kind == OS_Event_kind__key && ev->key_event.key == Key__w)
      {
        close_the_app = true;
        break;
      }
    }
    if (close_the_app) { break; }

    pcl_frame_update(&pcl);
    pcl_build_ui(font, &pcl, prev_frame_fps);

    /*
    UI_Build(os_get_client_area_dims(), os_get_mouse_pos(), font)
    {
      UI_Col()
      {
        for EachIndex(i, 200)
        {
          ui_text_f("SOme text here allla: %lld", i);
          
          // ui_next_width(ui_px(5));
          // ui_next_height(ui_px(5));
          // ui_next_b_color(golden());
          // UI_Box* golden_box = ui_box_make(UI_Box_flag__has_background, {});
        }
      }
    }
    */

    r_clear_handle(window_frame_buffer_target, black());
    ui_draw();

    if (show_debug_stuff)
    {
      d_draw_text_f("FPS: %lld", font, 32, v2f32(0, 0), magenta(), prev_frame_fps);
    }

    d_end_batching();
    
    ProfGroupF("Sumbit + Draw")
    {
      r_submit(window_frame_buffer_target, d_get_batch_list());
      r_present(window_frame_buffer_target, false);
    }
  
    os_frame_end();

    F64 frame_end_time_sec = os_get_time_for_timing_sec();

    prev_frame_fps = (U64)(1.0f/(frame_end_time_sec - frame_start_time_sec));

    ProfEndGroup();

    profiler_end_frame();
  }

  // Damian: Not releasing anything since who cares, the system will release all the stuff
  profiler_release();

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
              