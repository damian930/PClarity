// void OutputDebugStringF(const char* fmt, ...);

#include "core/core_include.h"
#include "core/core_include.cpp"

#include "profiler/profiler.h"
#include "profiler/profiler.cpp"

#include "os/win32.h"
#include "os/win32.cpp"

// #include "render/render.h"
// #include "render/render.cpp"

// #include "draw/draw.h"
// #include "draw/draw.cpp"

// #include "font_provider/font_provider.h"
// #include "font_provider/font_provider.cpp"

// #include "ui/ui_core.h"
// #include "ui/ui_core.cpp"

// #include "ui/widgets/ui_widgets.h"
// #include "ui/widgets/ui_widgets.cpp"


int main()
{
  os_init();
  allocate_thread_context();

  prof_init();

  ProfBeginGroupF("Start");
  os_sleep(1000);
  ProfEndGroup();

  prof_release();

  return 0;
}

//   ///////////////////////////////////////////////////////////
//   // - Window  
//   //
//   {
//     win32_state->window.window_class.cbSize        = sizeof(WNDCLASSEXA);
//     win32_state->window.window_class.style         = CS_HREDRAW|CS_VREDRAW;
//     win32_state->window.window_class.lpfnWndProc   = win32_proc;
//     win32_state->window.window_class.hInstance     = app_instance;
//     win32_state->window.window_class.hIcon         = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(1));
//     win32_state->window.window_class.hCursor       = LoadCursorW(0, IDC_ARROW);
//     win32_state->window.window_class.hbrBackground = Null;
//     win32_state->window.window_class.lpszMenuName  = Null;
//     win32_state->window.window_class.lpszClassName = L"pclarity_app_flopper_class_name";
//     win32_state->window.window_class.hIconSm       = Null;

//     ATOM wc_atom = RegisterClassExW(&win32_state->window.window_class);
//     Assert(wc_atom != 0);
    
//     win32_state->window.is_transparent = false;
//     win32_state->window.handle = CreateWindowExW(
//       WS_EX_NOREDIRECTIONBITMAP,
//       win32_state->window.window_class.lpszClassName,
//       L"PClarity",
//       WS_OVERLAPPEDWINDOW,
//       CW_USEDEFAULT, CW_USEDEFAULT,
//       800, 600,
//       Null,
//       Null,
//       app_instance, 
//       Null
//     );
//     Handle(win32_state->window.handle != 0);
//     ShowWindow(win32_state->window.handle, SW_NORMAL);
//   }

//   ///////////////////////////////////////////////////////////
//   // - App loop
//   //
//   R_Handle window_frame_buffer_target = r_attach_window(win32_state->window);
//   FP_Font font = fp_load_font(Str8FromC("../data/Roboto.ttf"), 32, rangeU64(0, (U64)u8_max + 1));

//   // State init
//   State* state = 0;
//   {
//     Arena* arena = arena_alloc(Megabytes(4));
//     state = ArenaPush(arena, State);
//     state->state_arena = arena;

//     for EachIndex(i, 64)
//     {
//       Node* node = ArenaPush(state->state_arena, Node);
//       node->value = i;
//       DllPushBack(&state->list, node);
//       state->list.count += 1;
//     }
//   }

//   for (;!os_window_should_close();)
//   {
//     Scratch scratch = get_scratch(0, 0);

//     os_frame_begin();
//     r_prepare_canvas(&window_frame_buffer_target);
//     d_begin_batching(window_frame_buffer_target);

//     { // State upate
//       if (state->is_delete_event)
//       {
//         state->is_delete_event = false;

//         Node* node = state->list.first;
//         for EachIndex(i, state->delete_event_node_index_to_delete)
//         { 
//           node = node->next;
//         }

//         Assert(node);
//         DllRemove(&state->list, node);
//         state->list.count -= 1;
//       }
//     }

//     // UI
//     UI_Build(os_get_client_area_dims(), os_get_mouse_pos(), font)
//     {
//       ui_next_width(ui_fit());
//       ui_next_height(ui_fit());
//       ui_next_border(2, blue());
//       ui_next_padding(10);
//       UI_Parent(ui_box_make(UI_Box_flag__padded_border, {}))
//       {
//         Node* node = state->list.first;
//         for EachIndex(node_index, state->list.count)
//         {
//           ui_next_width(ui_fit());
//           ui_next_height(ui_fit());
//           ui_next_border(1, green());
//           ui_next_padding(5);
//           ui_next_layout_x(); 
//           UI_Box* node_box = ui_box_make_f(UI_Box_flag__padded_border|UI_Box_flag__clickable, "Node id %p", node);

//           UI_Actions node_actions = ui_actions_from_box(node_box);
//           if (node_actions.is_hovered) { ui_box_set_b_color(node_box, nice_green()); }
//           if (node_actions.is_down) { ui_box_set_b_color(node_box, red()); }

//           UI_Parent(node_box)
//           {
//             ui_text_f("%lld", node->value);
//           }

//           // todo: Open up a context menu with a delete button on which you will delete the node from the state list
//           Str8 node_context_menu_id = str8_fmt(scratch.arena, "Node context menu id %p", node);
//           if (node_actions.is_clicked)
//           {
//             ui_set_context_menu_key(node_context_menu_id, v2f32(200, 200));
//           }

//           if (ui_is_context_menu_with_id_open(node_context_menu_id))
//           {
//             ui_begin_context_menu(node_context_menu_id);
//             {
//               ui_next_width(ui_fit());
//               ui_next_height(ui_fit());
//               ui_next_b_color(white());
//               UI_Box* context_menu = ui_box_make(UI_Box_flag__has_background, {});

//               UI_Parent(context_menu)
//               {
//                 ui_next_font_size(64);
//                 ui_next_font_color(magenta());
//                 UI_Actions button = ui_button_f("Delete node %lld", node_index);
//                 if (button.is_clicked)
//                 {
//                   state->is_delete_event = true;
//                   state->delete_event_node_index_to_delete = node_index;
//                   ui_reset_context_menu();
//                 }
//               }
//             }
//             ui_end_context_menu();
//           }

//           node = node->next;
//         }
//       }
//     }

//     r_clear_handle(window_frame_buffer_target, black());
//     ui_draw();

//     d_end_batching();
//     r_submit(window_frame_buffer_target, d_get_batch_list());
//     r_present(window_frame_buffer_target, false);
  
//     os_frame_end();
  
//     end_scratch(&scratch);
//   }

//   // Damian: Not releasing anything since who cares, the system will release all the stuff
//   profiler_release();

//   return 0;
// }

// ///////////////////////////////////////////////////////////
// // - Main helpers
// //
// // todo: Code for this is bad
// void OutputDebugStringF(const char* fmt, ...)
// {
//   #if DEBUG_MODE
//   va_list argptr;
//   va_start(argptr, fmt);
//   Scratch scratch = get_scratch(0, 0);
//   Data_buffer buffer = data_buffer_make(scratch.arena, 128);
//   int ret = vsprintf_s((char*)buffer.data, buffer.count, fmt, argptr);
//   if (ret >= 0 && ret < buffer.count)
//   {
//     OutputDebugStringA((char*)buffer.data);
//   } else { InvalidCodePath(); }
//   end_scratch(&scratch);
//   va_end(argptr);
//   #endif
// }
              