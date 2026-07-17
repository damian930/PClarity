#ifndef PCLARITY_H
#define PCLARITY_H

// TODO: Remove .cpp files from here

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

#include "pclarity/win32_data_retrival/win32_data_retrival.h"

enum PCL_Command {
  PCL_Command__NONE,
  PCL_Command__go_to_settings,
  PCL_Command__go_to_home,
  PCL_Command__close_the_app, // TODO:
  PCL_Command__new_main_font_size,
  PCL_Command__COUNT,
};

struct PCL_Command_node {
  PCL_Command command;
  PCL_Command_node* next;
  PCL_Command_node* prev;
};

struct PCL_Command_list {
  PCL_Command_node* first;
  PCL_Command_node* last;
  U64 count;
};

global Str8 pcl_command_name_for_user[PCL_Command__COUNT] = {
  Str8FromC("PCL_Command__NONE"),
  Str8FromC("Open Settings"),
  Str8FromC("Open Home"),
  Str8FromC("Exit"),
};

enum PCL_Color_name : U32 {
  PCL_Color_name__NONE,
  PCL_Color_name__main_background,
  PCL_Color_name__secondary_background,
  PCL_Color_name__item_selected,
  PCL_Color_name__change_main_font_size,
  PCL_Color_name__COUNT,
};

enum PCL_Menu {
  PCL_Menu__home,
  PCL_Menu__settings,
};

struct PCL_State {
  B32 is_command_window_open;
  PCL_Menu current_menu;

  // Frame data
  Arena* frame_arena;
  PCL_Command_list defered_commands_to_start_of_next_frame;
  ProcessInfoList gathered_process_data_this_frame;
  
  // TODO: This maybe should not be here, not sure, but here cause i just needed it somewhere
  // Table data
  F32 table_header_flex_values[64];
  U64 table_header_flex_value_count;

  // Config like data
  F32 main_font_size;

  // Data that is sometimes used along with commands
  F32 new_font_size;
};

void pcl_defer_command_to_start_of_next_frame(PCL_State* PCL, PCL_Command command)
{
  PCL_Command_node* command_node = ArenaPush(PCL->frame_arena, PCL_Command_node);
  command_node->command = command;
  PCL_Command_list* command_list = &PCL->defered_commands_to_start_of_next_frame;
  DllPushBack(command_list, command_node);
  command_list->count += 1;
}

// TODO: These should be the part of the PCL_State, i just havent moved them yet in there
global F32 pcl_font_size                  = 24.0f;
global R_Handle pcl_icon_settings         = r_zero_handle();
global R_Handle pcl_icon_home             = r_zero_handle();
global R_Handle pcl_icon_magnifying_glass = r_zero_handle();

V4F32 __pcl_g_color_values_for_names[PCL_Color_name__COUNT] = {
  rgba_from_hex(0x00000000) , // NONE
  rgba_from_hex(0x191432FF),
  rgba_from_hex(0x362753FF),
  rgba_from_hex(0x774ac9FF),
};

V4F32 pcl_color_from_name(PCL_Color_name color_name)
{
  if (color_name == PCL_Color_name__NONE || color_name == PCL_Color_name__COUNT) {
    Assert(0);
    return magenta();
  }
  V4F32 color = __pcl_g_color_values_for_names[color_name];
  return color;
}

// TODO: This should use dragging instead of just the mouse pos 
F32 pcl_ui_slider(F32 value, RangeF32 range_for_value, Str8 id)
{
  clamp_f32_inplace(&value, range_for_value.min, range_for_value.max);

  UI_Size size_x = ui_top_size_x();
  UI_Size size_y = ui_top_size_y();

  // Damian: Sizes from the outside are used here
  ui_next_padded_border(2, white());
  ui_next_hover_cursor(OS_Cursor__hand);
  UI_Box* slider_top_box = ui_box_make(id, UI_Box_flag__has_padding|UI_Box_flag__has_borders);
  UI_Parent(slider_top_box)
  {
    F32 value_normalised = (value - range_for_value.min) / (range_for_value.max - range_for_value.min);
    ui_next_width(ui_p_of_p(value_normalised));
    ui_next_height(ui_grow());
    ui_next_b_color(blue());
    ui_next_padding(5);
    UI_Parent(ui_box_make({}, UI_Box_flag__has_background|UI_Box_flag__has_padding))
    {
      ui_next_width(ui_grow());
      ui_next_height(ui_grow());
      ui_next_b_color(white());
      ui_box_make({}, UI_Box_flag__has_background);
    }
  }

  F32 new_value = value;
  UI_Actions slider_acts = ui_actions_from_box(slider_top_box);
  if (slider_acts.is_down)
  {
    UI_Box_data slider_box_data = ui_box_data_from_box(slider_top_box);
    if (slider_box_data.is_found)
    {
      F32 mouse_inside_slider_pos_normalised = (ui_get_mouse_pos().x - slider_box_data.rect.x) / (slider_box_data.rect.width - 2);
      new_value = lerp_f32(range_for_value.min, range_for_value.max, mouse_inside_slider_pos_normalised);
    }
  }
  clamp_f32_inplace(&new_value, range_for_value.min, range_for_value.max);

  return new_value;
}

PCL_State pcl_init()
{
  pcl_icon_settings         = r_load_texture_from_file(Str8FromC("../data/icons/gear.png"));
  pcl_icon_home             = r_load_texture_from_file(Str8FromC("../data/icons/house.png"));
  pcl_icon_magnifying_glass = r_load_texture_from_file(Str8FromC("../data/icons/magnifying_glass.png"));

  PCL_State state = {};
  state.current_menu   = PCL_Menu__home;
  state.main_font_size = 20;
  state.frame_arena = arena_alloc(Megabytes(64));

  // Icon, exe_name, start_up_time
  state.table_header_flex_values[state.table_header_flex_value_count++] = 1.0f;
  state.table_header_flex_values[state.table_header_flex_value_count++] = 0.5f;
  state.table_header_flex_values[state.table_header_flex_value_count++] = 0.5f;

  return state;
}

void pcl_frame_update(PCL_State* PCL)
{
  // Damian: Handling the defered commands 
  for (
    PCL_Command_node* command_node = PCL->defered_commands_to_start_of_next_frame.first; 
    command_node != 0;
    command_node = command_node->next  
  ) {
    switch (command_node->command)
    {
      case PCL_Command__COUNT: 
      default: { InvalidCodePath(); } break;
  
      case PCL_Command__NONE: {} break;  
      
      case PCL_Command__go_to_settings: 
      { 
        PCL->current_menu = PCL_Menu__settings;
      } break;
  
      case PCL_Command__go_to_home:
      {
        PCL->current_menu = PCL_Menu__home;
      } break;
  
      case PCL_Command__new_main_font_size:
      {
        PCL->main_font_size = PCL->new_font_size;
      } break;
    }
  }

  // Clearing per frame data before we start the new frame for real
  PCL->defered_commands_to_start_of_next_frame = {};
  arena_clear(PCL->frame_arena);

  B32 ctrl_p_down = false;
  for (OS_Event* ev = os_get_frame_event_list()->first; ev; ev = ev->next)
  {
    if (ev->kind == OS_Event_kind__key && ev->key_event.went_up && ev->key_event.key == Key__p && (ev->key_event.modifiers& OS_Event_modifier__control))
    {
      ctrl_p_down = true;
      os_consume_frame_event(ev);
      break;
    }
  }

  if (ctrl_p_down) 
  {
    PCL->is_command_window_open = true;
  }

  PCL->gathered_process_data_this_frame = Win32QueryProcessList(PCL->frame_arena);
}

// TODO: Finish this code here for edge cases and use it for the table in the pcl code
void pcl_scroll_bar(UI_Size size_x, UI_Size size_y, Str8 id, F32 outer_vp_size, F32 outer_content_size, F32 outer_vp_offset, F32* out_new_scroll, B32* is_new_offset)
{
  ui_next_width(size_x);
  ui_next_height(size_y);
  ui_next_b_color(nice_blue());
  ui_next_padding(3);
  UI_Box* scroll_bar = ui_box_make(id, UI_Box_flag__has_background|UI_Box_flag__has_padding);
  
  // NOTE(Damian):
  // Since logically all the vp and context sizes will be from the last frame,
  // we then can just get actions based on the last frame, update the offsets
  // then produce fresh scroll bar ui and then give the value to the called
  // and he then builds fresh offset in for his ui
  
  UI_Box_data scroll_bar_data = ui_box_data_from_box(scroll_bar);
  if (scroll_bar_data.is_found)
  {
    F32 scroll_bar_height = scroll_bar_data.rect.height;
    F32 inner_height = scroll_bar_data.inner_rect.height;

    F32 max_thumb_size = inner_height;
    
    #define THUMB_MIN_SIZE 25
    F32 thumb_size = (outer_vp_size / outer_content_size) * max_thumb_size;
    if (thumb_size > inner_height) { Handle(0); }
    if (thumb_size < THUMB_MIN_SIZE) { thumb_size = THUMB_MIN_SIZE; }

    F32 max_vp_offset = outer_content_size - outer_vp_size;
    F32 max_thumb_offset = inner_height - thumb_size;
    
    F32 thumb_offset = (outer_vp_offset / max_vp_offset) * max_thumb_offset;

    UI_Actions actions = ui_actions_from_box(scroll_bar);
    if (actions.is_down)
    {
      thumb_offset = ui_get_mouse_pos().y - scroll_bar_data.inner_rect.y;

      *is_new_offset  = true;
      *out_new_scroll = (thumb_offset / max_thumb_offset) * max_vp_offset;
    }

    UI_Parent(scroll_bar)
    {
      ui_spacer(ui_px(thumb_offset));

      ui_next_width(ui_grow());
      ui_next_height(ui_px(thumb_size));
      ui_next_b_color(magenta());
      UI_Box* thumb = ui_box_make({}, UI_Box_flag__has_background);
    }
  }
}

void pcl_do_ui(FP_Font font, PCL_State* PCL)
{
  Assert(IsZeroStruct(PCL->defered_commands_to_start_of_next_frame)); 

  ui_begin_build(os_get_client_area_dims(), os_get_mouse_pos(), font);

  ui_push_font_size(PCL->main_font_size);

  F32 icon_size = ui_top_font_size() * 3; // Damian: For now this is how it is, TODO

  ui_next_width(ui_p_of_p(1));
  ui_next_height(ui_p_of_p(1));
  UI_Row()
  {
    ui_next_width(ui_fit());
    ui_next_height(ui_grow());
    ui_next_b_color(pcl_color_from_name(PCL_Color_name__main_background));
    ui_next_padding(ui_top_font_size() * 0.25f);
    ui_next_layout_y();
    ui_next_alignment_x(UI_Alignment_x__center);
    UI_Box* box = ui_box_make_f("Left box id", UI_Box_flag__has_background|UI_Box_flag__has_padding);
    UI_Parent(box)
    {
      ui_next_width(ui_fit()); 
      ui_next_height(ui_fit()); 
      ui_next_padding(ui_top_font_size());
      ui_next_border(2, pcl_color_from_name(PCL_Color_name__item_selected));
      ui_next_hover_cursor(OS_Cursor__hand);
      UI_Box* home_button = ui_box_make(Str8FromC("Navigation rail home button"), UI_Box_flag__has_background|UI_Box_flag__has_borders);
      UI_Parent(home_button)
      {
        ui_image(pcl_icon_home, icon_size, icon_size);
        
        UI_Actions home_button_acts = ui_actions_from_box(home_button);
        if (home_button_acts.is_clicked)
        {
          pcl_defer_command_to_start_of_next_frame(PCL, PCL_Command__go_to_home);
        }
        if (home_button_acts.is_hovered) {
          ui_set_box_b_color(home_button, pcl_color_from_name(PCL_Color_name__item_selected));
        }
      }
  
      ui_spacer(ui_grow());
  
      ui_next_width(ui_fit()); 
      ui_next_height(ui_fit()); 
      ui_next_padding(ui_top_font_size());
      ui_next_border(2, pcl_color_from_name(PCL_Color_name__item_selected));
      ui_next_hover_cursor(OS_Cursor__hand);
      UI_Box* settings_button = ui_box_make(Str8FromC("Navigation rail setting button"), UI_Box_flag__has_background|UI_Box_flag__has_borders);
      UI_Parent(settings_button)
      {
        ui_image(pcl_icon_settings, icon_size, icon_size);
  
        UI_Actions settings_button_acts = ui_actions_from_box(settings_button);
        if (settings_button_acts.is_clicked)
        {
          pcl_defer_command_to_start_of_next_frame(PCL, PCL_Command__go_to_settings);
        }
        if (settings_button_acts.is_hovered) {
          ui_set_box_b_color(settings_button, pcl_color_from_name(PCL_Color_name__item_selected));
        }
      }
    }
  
    ui_next_width(ui_px(1));
    ui_next_height(ui_grow());
    ui_next_b_color(magenta());
    ui_box_make_f("Navigation rail and main page separator", UI_Box_flag__has_background);
  
    // Menu box
    ui_next_width(ui_grow());
    ui_next_height(ui_grow());
    ui_next_b_color(pcl_color_from_name(PCL_Color_name__main_background));
    ui_next_extra_flags(UI_Box_flag__has_background|UI_Box_flag__has_padding);
    ui_next_padding(ui_top_font_size()); // todo: Use a scale here
    UI_Col()
    {
      if (PCL->current_menu == PCL_Menu__settings)
      {
        ui_next_width(ui_grow());
        ui_next_height(ui_rem(1));
        F32 new_font_size = pcl_ui_slider(PCL->main_font_size, rangeF32(4, 64), Str8FromC("Test slider id"));
        
        pcl_defer_command_to_start_of_next_frame(PCL, PCL_Command__new_main_font_size);
        PCL->new_font_size = new_font_size;
  
        ui_next_font_size(64);
        ui_label_f("Settings menu here");
      }
      else if (PCL->current_menu == PCL_Menu__home)
      {
        ui_next_font_size(32);
        ui_next_font_color(magenta());
        ui_text_f("Application data");

        ui_spacer(ui_rem(0.25));

        // TODO: There is a bug that the borders are not visible
        // DD: The top table box
        ui_next_width(ui_grow());
        ui_next_height(ui_grow());
        ui_next_padding_diff(20, 20, 10, 10);
        ui_next_padded_border(1, green());
        ui_next_padding(5);
        ui_next_extra_flags(UI_Box_flag__has_padding|UI_Box_flag__has_borders);
        UI_Col()
        {
          // Table + vertical slider on the left of the table
          ui_next_width(ui_grow()); ui_next_height(ui_grow());
          UI_Row()
          {
            ui_next_width(ui_grow());
            ui_next_height(ui_grow());
            UI_Parent(ui_box_make({}, 0))
            {
              // TODO: Move these out, this is here for now
              #define ROW_HEIGHT_SCALER 3 
              #define RESIZER_VISIBLE_WIDTH 3
      
              // TODO: Clay fucked up auto generated ids here, need to introduce custom id scopes
              ui_next_width(ui_grow());
              ui_next_height(ui_grow());
              ui_next_layout_y();
              ui_next_b_color(black());
              UI_Box* table_box = ui_box_make_f("table_boxflkdfjlsdk", UI_Box_flag__clip); // TODO: Change the name here

              ui_box_set_clip_offset(table_box, ui_get_prev_build_scroll_for_box(table_box));
              if (ui_actions_from_box(table_box).is_hovered)
              {
                // TODO: Add shift modifier here to be able to scroll to the right and back
                F32 table_scroll_this_frame = 0.0f;
                Axis2 scroll_axis = Axis2__y;
                for (OS_Event* ev = os_get_frame_event_list()->first; ev; ev = ev->next)
                {
                  if (ev->kind == OS_Event_kind__wheel)
                  {
                    table_scroll_this_frame = ev->wheel_event.scroll_data;
                    if (ev->wheel_event.modifiers & OS_Event_modifier__shift) { scroll_axis = Axis2__x; }
                    os_consume_frame_event(ev);
                    break;
                  }
                }

                F32 prev_frame_offset = ui_get_prev_build_scroll_for_box(table_box).v[scroll_axis];
                F32 new_frame_offset = prev_frame_offset + (table_scroll_this_frame * 10);
                
                // Clamping offset to stay valid 
                if (scroll_axis != Axis2__x)
                { 
                  V2F32 inner_dims = ui_get_content_dims_from_box(table_box);
                  V2F32 dims = ui_box_data_from_box(table_box).rect.dims;

                  F32 max_offset = inner_dims.v[scroll_axis] - dims.v[scroll_axis]; 

                  if (new_frame_offset > 0.0f) { new_frame_offset = 0.0f; }
                  if (new_frame_offset < -max_offset) { new_frame_offset = -max_offset; }
                }

                ui_box_set_clip_offset_for_axis(table_box, new_frame_offset, scroll_axis);
              }

              UI_Box_data table_box_data = ui_box_data_from_box(table_box);
              if (table_box_data.is_found)
              {
                if (PCL->table_header_flex_value_count != 0) UI_Parent(table_box)
                {
                  // TODO: This might cause some weird looking bugs since in clay padding adds to the size of things
                  //       where just border doesnt
                  // This here doesnt account for any border or anything like that, this should
                  U64 n_resizers_per_row = PCL->table_header_flex_value_count - 1;
                  F32 table_width        = table_box_data.rect.width;
                  F32 table_height       = table_box_data.rect.height;
                  F32 space_for_headers  = (F32)(table_width - (n_resizers_per_row * RESIZER_VISIBLE_WIDTH));
        
                  F32 headers_total_flex_value = 0.0f;
                  for EachIndex(header_index, PCL->table_header_flex_value_count)
                  {
                    headers_total_flex_value += PCL->table_header_flex_values[header_index];
                  }
        
                  // Floating resizers
                  {
                    B32 is_pending_resizer_drag = false;
                    U64 pending_resizer_index   = 0;
                    F32 pending_resizer_drag    = 0.0;
                    
                    F32 offset_x = 0.0f;
                    for EachIndex(header_index, PCL->table_header_flex_value_count)
                    {
                      if (header_index == PCL->table_header_flex_value_count - 1) { continue; }
        
                      F32 flex_norm = PCL->table_header_flex_values[header_index] / headers_total_flex_value;
                      offset_x += space_for_headers * flex_norm;
                      offset_x += RESIZER_VISIBLE_WIDTH; 
        
                      #define RESIZER_INVISIBLE_WIDTH 6
                      ui_next_width(ui_px(RESIZER_INVISIBLE_WIDTH));
                      ui_next_height(ui_rem(ROW_HEIGHT_SCALER));
                      ui_next_hover_cursor(OS_Cursor__horizontal_resize);
                      ui_next_b_color(transparent());
                      UI_Box* resizer = ui_box_make_f("Data table resizer %lld", UI_Box_flag__has_background|UI_Box_flag__floating, header_index);
                      resizer->clay_element_config.floating.offset.x = offset_x - ((F32)RESIZER_VISIBLE_WIDTH / 2) - ((F32)RESIZER_INVISIBLE_WIDTH / 2); 
        
                      // TODO: There i a bug here, when the left resizer gets dragged, the right one moves as well
                      //       This is also what is causing the separator to look smaller sometimes for a couple of pixels
                      UI_Actions resizer_actions = ui_actions_from_box(resizer);
                      if (resizer_actions.is_down)
                      {
                        F32 drag = ui_get_mouse_pos().x - ui_get_prev_mouse_pos().x;
        
                        is_pending_resizer_drag = true;
                        pending_resizer_index   = header_index;
                        pending_resizer_drag    = drag;
                      }
                    }
        
                    if (is_pending_resizer_drag)
                    {
                      F32 old_left_flex  = PCL->table_header_flex_values[pending_resizer_index + 0];
                      F32 old_right_flex = PCL->table_header_flex_values[pending_resizer_index + 1];
                      
                      F32 old_left_px = (old_left_flex / headers_total_flex_value) * space_for_headers;
        
                      F32 flex_change_on_left = old_left_flex * (pending_resizer_drag / old_left_px);
        
                      F32 new_left_flex  = old_left_flex + flex_change_on_left;
                      F32 new_right_flex = old_right_flex - flex_change_on_left;
        
                      if (new_left_flex > 0.0f && new_right_flex > 0.0f) 
                      {
                        PCL->table_header_flex_values[pending_resizer_index + 0] = new_left_flex;
                        PCL->table_header_flex_values[pending_resizer_index + 1] = new_right_flex;
                      }
                    }
                  }
        
                  // TODO: Assert that the flex value from before is the same as now
        
                  // Damian: Building table headers
                  ui_next_size_x(ui_grow());
                  ui_next_size_y(ui_rem(ROW_HEIGHT_SCALER));
                  UI_Row()
                  {
                    for EachIndex(header_index, PCL->table_header_flex_value_count)
                    {
                      F32 flex_norm = PCL->table_header_flex_values[header_index] / headers_total_flex_value;
        
                      ui_next_width(ui_px(flex_norm * space_for_headers));
                      ui_next_height(ui_grow()); 
                      UI_Box* header_box = ui_box_make_f("Table header %lld", UI_Box_flag__dont_draw_overflow, header_index);
                      UI_Parent(header_box)
                      {
                        // Damian: This is for the ui_text_ellipsed to work, this api is not great yet, but it works
                        UI_Height(ui_grow()) 
                        UI_Width(ui_grow())
                        UI_FontSize(48)
                        {
                          if (0) {}
                          else if (header_index == 0)  { ui_text_ellipsed_f("Icon"); } 
                          else if (header_index == 1)  { ui_text_ellipsed_f("Exe Name"); } 
                          else if (header_index == 2)  { ui_text_ellipsed_f("Start Time"); } 
                        }
                      }
        
                      if (header_index != PCL->table_header_flex_value_count - 1)
                      {
                        ui_next_width(ui_px(RESIZER_VISIBLE_WIDTH));
                        ui_next_height(ui_grow());
                        ui_next_b_color(nice_blue());
                        ui_box_make({}, UI_Box_flag__has_background);
                      }
                    }
                  }
        
                  // Horizontal separator between the headers and the table body or rows
                  { 
                    ui_next_width(ui_grow());
                    ui_next_height(ui_px(1));
                    ui_next_b_color(orange());
                    UI_Box* _box = ui_box_make_n(UI_Box_flag__has_background, {});
                  }
                  
  
                  // Damian: Building table rows
                  {
                    U64 row_index = 0;
                    for (
                      ProcessInfoNode* process_data = PCL->gathered_process_data_this_frame.first;
                      process_data != 0;
                      process_data = process_data->next, row_index += 1
                    ) {
                      ui_next_width(ui_grow());
                      ui_next_height(ui_rem(ROW_HEIGHT_SCALER));
                      ui_next_extra_flags(UI_Box_flag__has_borders|UI_Box_flag__has_padding);
                      UI_Row()
                      {
                        for EachIndex(header_index, PCL->table_header_flex_value_count)
                        {
                          F32 flex_norm = PCL->table_header_flex_values[header_index] / headers_total_flex_value;
          
                          ui_next_width(ui_px(flex_norm * space_for_headers));
                          ui_next_height(ui_grow()); 
                          UI_Parent(ui_box_make({}, UI_Box_flag__dont_draw_overflow))
                          {
                            switch(header_index)
                            {
                              default: { InvalidCodePath(); } break;
          
                              case 0: 
                              {
                                ui_text_f("PID: %d", process_data->pid);
                              } break;
                              
                              case 1:
                              {
                                ui_text_f("PPID: %d", process_data->ppid);
                              }
                              
                              case 2: 
                              {
                                ui_text_f("Time:%.*s", Str8FmtArg(process_data->create_time));
                              } break;
                            }
                          }
          
                          if (header_index != PCL->table_header_flex_value_count - 1)
                          {
                            ui_next_width(ui_px(RESIZER_VISIBLE_WIDTH));
                            ui_next_height(ui_grow());
                            ui_next_b_color(nice_blue());
                            ui_box_make({}, UI_Box_flag__has_background);
                          }
                        }
                      }
                    
                      ui_next_width(ui_grow());
                      ui_next_height(ui_rem(0.1f));
                      ui_next_b_color(green());
                      ui_box_make({}, UI_Box_flag__has_background);
                    }
                  }
                  
                }
              }
            }

            ui_spacer(ui_rem(0.25f));

            // TODO: Make a scroll bar in place here
            //       Dont forget about single source of truth thought, either aply scroll next frame
            //       or build the scroll bar here, but do the actions in the beginnn when you do the 
            //       wheel scroll as well to have a single source of truth untouched
            {
              ui_next_width(ui_rem(2));
              ui_next_height(ui_grow());
              ui_next_b_color(red());
              ui_next_padding(3);
              UI_Box* scroll_bar = ui_box_make_f("Scroll bar", UI_Box_flag__has_background|UI_Box_flag__has_padding);
              
              UI_Box_data scroll_bar_data = ui_box_data_from_box(scroll_bar);
              if (scroll_bar_data.is_found)
              {
                F32 scroll_bar_height = scroll_bar_data.rect.height;
                F32 inner_height = scroll_bar_data.inner_rect.height;

                F32 thumb_size = ui_top_font_size() * 2;
                if (thumb_size > inner_height) { Handle(0); }




                // Steps:
                // - Get thumb size
                // - Then get thumb offset
                // - Then get the offset for the thing on the outside from thumb offset
              }

              // todo: Get the data for scroll bar

              // Do the events to have new scroll bar in the frame
              // Then build the thing

              // TODO: Get the events

              // TODO: For now just give out relative vp offset to the outside
              UI_Parent(scroll_bar)
              {
                

  
              }


            }
            
          }

          // TODO: Horizontal sider at the bottom

          // TODO: Table + sliders

          // TODO: Dont use table widht here
          
        }
      
      }
      else {
        InvalidCodePath();
      }
    }

    // Spawning the command window on top of everything else 
    if (PCL->is_command_window_open)
    {
      UI_Parent(ui_get_root())
      {
        ui_next_width(ui_grow());
        ui_next_height(ui_grow());
        ui_next_alignment_x(UI_Alignment_x__center);
        ui_next_alignment_y(UI_Alignment_y__center);
        ui_next_extra_flags(UI_Box_flag__floating);
        UI_Parent(ui_box_make({}, 0))
        {
          V4F32 orange_back_color   = rgba_from_hex(0x4f3417FF); 
          V4F32 orange_border_color = rgba_from_hex(0xe29328FF);
          V4F32 dark_back_color     = rgba_from_hex(0x1a1a1aFF);

          UI_BorderColor(orange_border_color)
          {
            ui_next_width(ui_p_of_p(0.5f));
            ui_next_height(ui_p_of_p(0.85f));
            ui_next_border_width(4);
            ui_next_padding(4);
            ui_next_extra_flags(UI_Box_flag__has_borders|UI_Box_flag__has_padding);
            UI_Col()
            {
              ui_next_width(ui_p_of_p(1));
              ui_next_height(ui_fit()); // todo: This should be the size of the text that will be used to input the text in there
              ui_next_border_width(4);
              ui_next_padding(2 * 4);
              ui_next_b_color(orange_back_color);
              ui_next_extra_flags(UI_Box_flag__has_borders|UI_Box_flag__has_padding|UI_Box_flag__has_background);
              UI_Parent(ui_box_make({}, 0))
              {
                UI_FontSize(ui_top_font().size)
                {
                  Scratch scratch = get_scratch(0, 0);
    
                  static U8 text_buffer[32] = {};
                  static U64 text_buffer_size = {};
                  static U64 cursor_pos = {};
                  static U64 section_start_pos = {};

                  ui_next_b_color(blue());
                  ui_next_extra_flags(UI_Box_flag__has_background);
                  UI_Text_op_list op_list = {};
                  UI_Col()
                  {
                    UI_Text_op_list op_list_ = ui_text_edit_box(scratch.arena, true, ui_px(300), text_buffer, text_buffer_size, ArrayCount(text_buffer), cursor_pos, section_start_pos, Str8FromC("Test text edit field"));
                    op_list = op_list_;
                  }

                  ui_aply_text_ops(op_list, text_buffer, ArrayCount(text_buffer), &text_buffer_size, &cursor_pos, &section_start_pos, Null, Null);
    
                  end_scratch(&scratch);
                }
              }

              // todo: Here just spawn each of the command thing to show the user to use
              
              ui_next_width(ui_grow());
              ui_next_height(ui_grow());
              ui_next_b_color(dark_back_color);
              UI_Box* command_scroll_list = ui_box_make({}, UI_Box_flag__has_background|UI_Box_flag__clip);
              UI_Parent(command_scroll_list)
              { 
                for EachEnumRange(command, PCL_Command, PCL_Command__go_to_settings, PCL_Command__COUNT)
                {
                  UI_Col()
                  {
                    ui_label(pcl_command_name_for_user[command]);
                    // ui_label_f("Some text in smaller font that tells the user in detail about the command");
                  }
                }
              }

              // TODO:
              // UI_Row()
              // {
              //   // todo: Here is the list of the command that might be used
              
              //   // todo: Here will be the slider for the list
              // }
            }

          }


        }
      }
    }
  }

  ui_end_build();
}

#endif









