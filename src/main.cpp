void OutputDebugStringF(const char* fmt, ...);

#include "core/core_include.h"
#include "core/core_include.cpp"

#include "os/win32.h"
#include "os/win32.cpp"

#include "profiler/profiler.h"
#include "profiler/profiler.cpp"

#include "render/render.h"
#include "render/render.cpp"

#include "font_provider/font_provider.h"
#include "font_provider/font_provider.cpp"

#include "draw/draw.h"
#include "draw/draw.cpp"

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

/*
void handle_ui_dll_hot_reload(U64* last_dll_write_time, OS_Handle* ui_dll_handle, App_ui_construct_ft** app_ui_construct_fp)
{
  OS_FileOpenClose(dll_file, UI_DLL_PATH, OS_File_access__visible_read)
  {
    if (os_file_is_valid(dll_file))
    {
      OS_File_properties props = os_file_get_properties(dll_file);
      U64 new_write_time = props.last_write_time;
      if (new_write_time != *last_dll_write_time)
      {
        os_unload_dll(ui_dll_handle);
        os_file_copy(UI_DLL_PATH, UI_DLL_FOR_HOT_RELOAD_PATH);
        *ui_dll_handle = os_load_dll(UI_DLL_FOR_HOT_RELOAD_PATH);
        if (os_handle_is_valid(*ui_dll_handle))
        {
          Str8 proc_name = Str8FromC(Stringify(AppUIConstruct_FuncName));
          App_ui_construct_ft* new_ui_proc = (App_ui_construct_ft*)os_load_proc_from_dll(*ui_dll_handle, proc_name);
          if (new_ui_proc) {
            *app_ui_construct_fp = new_ui_proc;
            str8_printf("Reloaded the ui dll \n");
          }
        }
        *last_dll_write_time = new_write_time;
      }
    }
  }
}
*/

PCL_BUILD_UI_FUNC_DEF(pcl_build_ui__stub) { }

///////////////////////////////////////////////////////////
// DD: Stuff fror ui dll reload 
#define UI_DLL_PATH__CSTR                "__main_ui.dll"
#define UI_DLL_FOR_HOT_RELOAD_PATH__CSTR "__main_ui_copy_for_hot_reload.dll"

#define UI_DLL_PATH                Str8FromC(UI_DLL_PATH__CSTR)
#define UI_DLL_FOR_HOT_RELOAD_PATH Str8FromC(UI_DLL_FOR_HOT_RELOAD_PATH__CSTR)

HMODULE dll_handle               = 0;
U64 last_recorded_dll_write_time = 0;
PCL_Build_ui_func* pcl_build_ui  = pcl_build_ui__stub;

///////////////////////////////////////////////////////////
// Main 
///////////////////////////////////////////////////////////

int WinMain(HINSTANCE app_instance, HINSTANCE __not_used__, LPSTR cmd, int show)
{
  // Layers we allocate for the runtime 
  prof_init();
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

  U64 frame_counter  = 0;
  U64 prev_frame_fps = 0;

  for (;!os_window_should_close(); frame_counter += 1) 
  {
    // Reload for the ui dll
    OS_FileOpenClose(dll_file, UI_DLL_PATH, OS_File_access__visible_read)
    {
      if (os_file_is_valid(dll_file))
      {
        OS_File_props props = os_file_get_props(dll_file);
        U64 new_write_time = props.last_write_time;
        if (new_write_time != last_recorded_dll_write_time)
        {
          // DD: Unloading 
          pcl_build_ui = pcl_build_ui__stub;
          FreeModule(dll_handle);
          os_file_copy(UI_DLL_PATH, UI_DLL_FOR_HOT_RELOAD_PATH);

          dll_handle = LoadLibraryA(UI_DLL_FOR_HOT_RELOAD_PATH__CSTR); 
          if (dll_handle)
          {
            PCL_Build_ui_func* new_pcl_build_ui = (PCL_Build_ui_func*)GetProcAddress(dll_handle, Stringify(PCL_BUILD_UI__FUNC_FOR_EXPORT__NAME));
            if (new_pcl_build_ui)
            {
              pcl_build_ui = new_pcl_build_ui;
            }
          }
          last_recorded_dll_write_time = new_write_time;
        }
      }
    }

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

    r_clear_handle(window_frame_buffer_target, black());

    PCL_UI_Dll_context dll_context = {};
    dll_context.os_state            = os_get_state();
    dll_context.thread_context      = get_thread_context();
    dll_context.prof_state          = prof_get_state();
    dll_context.r_state             = r_get_state();
    dll_context.font_provider_state = fp_get_state();
    dll_context.draw_state          = d_get_state();
    dll_context.ui_state            = ui_get_state();

    PCL_Debug_data_for_ui pcl_debug_data = {};
    pcl_debug_data.fps         = prev_frame_fps;
    pcl_debug_data.widnow_dims = os_get_client_area_dims();

    Assert(pcl_build_ui);
    if (pcl_build_ui) { pcl_build_ui(font, &pcl, dll_context, pcl_debug_data);  }

    /*
    UI_Build(os_get_client_area_dims(), os_get_mouse_pos(), font)
    {
      ui_next_width(ui_px(250));
      ui_next_height(ui_px(250));
      ui_next_corner_r(25);
      ui_next_border(5, green());
      ui_next_inner_softness(1);
      ui_next_outer_softness(1);
      ui_next_b_color(red());
      UI_Box* box = ui_box_make(UI_Box_flag__has_borders|UI_Box_flag__has_rounded_corners, {});
    }
    */

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
  }

  // Damian: Not releasing anything since who cares, the system will release all the stuff
  prof_release();

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
              