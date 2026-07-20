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

  U64 frame_counter  = 0;
  U64 prev_frame_fps = 0;
  for (;!os_window_should_close(); frame_counter += 1)
  {
    ProfBeginGroupF("App frame %d", frame_counter);

    F64 frame_start_time_sec = os_get_time_for_timing_sec();
    
    os_frame_begin();
    r_prepare_canvas(&window_frame_buffer_target);
    d_begin_batching(window_frame_buffer_target);

    for (OS_Event* ev = os_get_frame_event_list()->first; ev; ev = ev->next)
    {
      if (ev->kind == OS_Event_kind__key && ev->key_event.key == Key__f1 && ev->key_event.went_down)
      {
        show_debug_stuff = ToggleBool(show_debug_stuff);
        os_consume_frame_event(ev);
        break;
      }
    }

    /*
    UI_Build(os_get_client_area_dims(), os_get_mouse_pos(), font)
    {
      ui_next_width(ui_grow());
      ui_next_height(ui_grow());
      ui_next_padded_border(3, nice_green());
      ui_next_padding(15);
      ui_next_child_gap(5);
      ui_next_layout_x();
      UI_Box* top_box = ui_box_make(UI_Box_flag__has_padding|UI_Box_flag__has_borders|UI_Box_flag__has_child_gap, {});
      UI_Parent(top_box)
      {
        UI_Box* clip_box = ui_box_make(UI_Box_flag__clip, Str8FromC("Clip box id"));
        UI_Parent(clip_box)
        {
          UI_Parent(ui_box_make(0, Str8FromC("id 1")))
          UI_Parent(ui_box_make(0, Str8FromC("id 2")))
          UI_Parent(ui_box_make(0, Str8FromC("id 3")))
          UI_Parent(ui_box_make(0, Str8FromC("id 4")))
          UI_Parent(ui_box_make(0, Str8FromC("id 5")))
          UI_Parent(ui_box_make(0, Str8FromC("id 11")))
          UI_Parent(ui_box_make(0, Str8FromC("id 12")))
          UI_Parent(ui_box_make(0, Str8FromC("id 13")))
          UI_Parent(ui_box_make(0, Str8FromC("id 14")))
          UI_Parent(ui_box_make(0, Str8FromC("id 15")))
          UI_Parent(ui_box_make(0, Str8FromC("id 111")))
          UI_Parent(ui_box_make(0, Str8FromC("id 122")))
          UI_Parent(ui_box_make(0, Str8FromC("id 133")))
          UI_Parent(ui_box_make(0, Str8FromC("id 144")))
          UI_Parent(ui_box_make(0, Str8FromC("id 155")))
          UI_Parent(ui_box_make(0, Str8FromC("id 1111")))
          UI_Parent(ui_box_make(0, Str8FromC("id 1222")))
          UI_Parent(ui_box_make(0, Str8FromC("id 1333")))
          UI_Parent(ui_box_make(0, Str8FromC("id 1444")))
          UI_Parent(ui_box_make(0, Str8FromC("id 1555")))
          {
            UI_Row()
            {
              for EachIndex(i, 25)
              {
                ui_next_width(ui_fit());
                ui_next_height(ui_fit());
                ui_next_padded_border(3, nice_blue());
                ui_next_padding(10);
                ui_next_child_gap(5);
                ui_next_extra_flags(UI_Box_flag__has_padding|UI_Box_flag__has_borders|UI_Box_flag__has_child_gap);
                UI_Col()
                {
                  for EachIndex(j, 25)
                  {
                    UI_Wrapper()
                    {
                      ui_next_width(ui_px(25));
                      ui_next_height(ui_px(25));
                      ui_next_b_color(red());
                      UI_Box* red_box = ui_box_make(UI_Box_flag__has_background, {});
                    }
                  }
                }
              }
            }
          }
        }
      
        UI_Box_clip_data box_clip_data = ui_box_clip_data_from_box(clip_box);
        if (box_clip_data.is_found)
        {
          B32 is_new_offset = false;
          F32 new_offset = 0.0f;
          pcl_scroll_bar(ui_px(50), ui_grow(), Axis2__y, Str8FromC("Scroll bar"), box_clip_data.viewport_dims.y, box_clip_data.content_dims.y, -ui_clip_offset_from_box(clip_box).y, &new_offset, &is_new_offset);

          if (is_new_offset)
          {
            ui_box_set_clip_offset_y(clip_box, -new_offset);
          }
        }
        
      }
    }
    */

    pcl_frame_update(&pcl);
    pcl_do_ui(font, &pcl);
    
    /*
    UI_Build(os_get_client_area_dims(), os_get_mouse_pos(), font)
    {
      Str8 box_id = Str8FromC("Box test id");
      UI_Box_data box_data = ui_box_data_from_id(box_id);
      OutputDebugStringF("Rect: .x = %.3f, .y = %.3f, .width = %.3f, .height = %.3f \n", box_data.rect.x, box_data.rect.y, box_data.rect.width, box_data.rect.height);

      if (make)
      {
        ui_next_floating_fixed_pos_x(50);
        ui_next_floating_fixed_pos_y(50);
        ui_next_width(ui_px(50));
        ui_next_height(ui_px(50));
        ui_next_b_color(nice_blue());
        ui_next_padding(5);
        UI_Box* box = ui_box_make(box_id, UI_Box_flag__floating|UI_Box_flag__has_background|UI_Box_flag__has_padding);
  
        UI_Parent(box)
        {
          ui_next_width(ui_grow());
          ui_next_height(ui_grow());
          ui_next_b_color(nice_green());
          UI_Box* nested = ui_box_make({}, UI_Box_flag__has_background);
        }
        
        UI_Actions box_acts = ui_actions_from_box(box);
        if (box_acts.is_down)
        {
          ui_box_set_b_color(box, white());
        }
        else if (box_acts.is_hovered)
        {
          ui_box_set_b_color(box, red());
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
    r_submit(window_frame_buffer_target, d_get_batch_list());
    r_present(window_frame_buffer_target, false);
  
    os_frame_end();

    F64 frame_end_time_sec = os_get_time_for_timing_sec();

    prev_frame_fps = (U64)(1.0f/(frame_end_time_sec - frame_start_time_sec));
    
    ProfEndGroup();
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
              