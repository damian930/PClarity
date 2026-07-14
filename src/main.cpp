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

#include "pclarity/pclarity_ui.h"

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
  
  // Arena* table_arena = arena_alloc(Megabytes(4));
  // Table_state* table_state = ArenaPush(table_arena, Table_state);
  // {
  //   table_add_header(table_arena, table_state, 0.4f, Str8FromC("H1"));
  //   table_add_header(table_arena, table_state, 0.4f, Str8FromC("H2"));
  //   table_add_header(table_arena, table_state, 0.2f, Str8FromC("H3"));

  //   Table_row_entry_list* row_1 = table_add_row(table_arena, table_state);
  //   table_add_col_data_to_row(table_arena, row_1, Str8FromC("Row1_H1_data"));
  //   table_add_col_data_to_row(table_arena, row_1, Str8FromC("Row1_H2_data"));
  //   table_add_col_data_to_row(table_arena, row_1, Str8FromC("Row1_H3_data"));
  
  //   Table_row_entry_list* row_2 = table_add_row(table_arena, table_state);
  //   table_add_col_data_to_row(table_arena, row_2, Str8FromC("Row2_H1_data"));
  //   table_add_col_data_to_row(table_arena, row_2, Str8FromC("Row2_H2_data"));
  //   table_add_col_data_to_row(table_arena, row_2, Str8FromC("Row2_H3_data"));
  
  //   Table_row_entry_list* row_3 = table_add_row(table_arena, table_state);
  //   table_add_col_data_to_row(table_arena, row_3, Str8FromC("Row3_H1_data"));
  //   table_add_col_data_to_row(table_arena, row_3, Str8FromC("Row3_H2_data"));
  //   table_add_col_data_to_row(table_arena, row_3, Str8FromC("Row3_H3_data"));
  // }

  UI_Table_config table_conf = {};
  {
    table_conf.row_size_in_pixels        = 100;
    table_conf.flex_values_for_headers[0]    = 150;
    table_conf.flex_values_for_headers[1]    = 250;
    table_conf.flex_values_for_headers[2]    = 100;
    table_conf.flex_values_for_headers_count = 3;
    table_conf.border_color              = green();
    table_conf.border_around_width       = 2;
  }

  for (;!os_window_should_close();)
  {
    F64 frame_start_time_sec = os_get_time_for_timing_sec();
    
    os_frame_begin();
    r_prepare_canvas(&window_frame_buffer_target);
    d_begin_batching(window_frame_buffer_target);

    // pcl_frame_update(&pcl);
    // pcl_do_ui(font, &pcl);
    // test_table_ui(font);
    // test_code_for_table_api(font);
    table_do_ui_build(&table_conf, font, 500, 500);

    // ui_begin_build(os_get_client_area_dims(), os_get_mouse_pos(), font);
    // ui_push_font_size(24);
    // {
    //   static F32 box_width = 100;
    //   ui_next_width(ui_px(200));
    //   ui_next_height(ui_px(50));
    //   box_width = pcl_ui_slider(box_width, rangeF32(10, 200), Str8FromC("Slider id"));

    //   ui_next_width(ui_px(box_width));
    //   ui_next_height(ui_fit());
    //   ui_next_b_color(nice_blue());
    //   ui_next_extra_flags(UI_Box_flag__has_background);
    //   UI_Wrapper()
    //   {
    //     ui_next_width(ui_grow());
    //     ui_next_height(ui_px(50));
    //     ui_label_ellipsed(Str8FromC("fsdlfjsdklfsdklfsdjlfksdf"));
    //   }

    // }
    // ui_end_build();

    r_clear_handle(window_frame_buffer_target, black());
    ui_draw();

    d_end_batching();
    r_submit(window_frame_buffer_target, d_get_batch_list());
    r_present(window_frame_buffer_target, false);
  
    os_frame_end();

    F64 frame_end_time_sec = os_get_time_for_timing_sec();

    OutputDebugStringF("Frame time sec: %f\n", frame_end_time_sec - frame_start_time_sec);
    OutputDebugStringF("FPS:            %f\n", 1.0f/(frame_end_time_sec - frame_start_time_sec));
    OutputDebugStringF("\n");
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
              