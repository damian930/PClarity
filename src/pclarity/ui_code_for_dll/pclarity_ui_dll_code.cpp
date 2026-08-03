#ifndef PCLARITY_UI_DLL_CODE_CPP
#define PCLARITY_UI_DLL_CODE_CPP

#include "pclarity/pclarity_commons.h"
#include "pclarity/pclarity_commons.cpp"

#include "pclarity/ui_code_for_dll/pclarity_ui_dll_code.h"

///////////////////////////////////////////////////////////
// - Code to export outside the dll 
//
extern "C" __declspec( dllexport )
void PCL_BUILD_UI__FUNC_FOR_EXPORT__NAME(FP_Font font, PCL_State* pcl, F64 prev_frame_fps, PCL_UI_Dll_context dll_context) 
{
  ProfBeginFunc();

  // DD: Setting states for all the layers since we are in a dll
  os_set_state(dll_context.os_state);
  set_thread_context(dll_context.thread_context);
  r_set_state(dll_context.r_state);
  fp_set_state(dll_context.font_provider_state);
  d_set_state(dll_context.draw_state);
  ui_set_state(dll_context.ui_state);

  Assert(IsZeroStruct(pcl->defered_commands_to_start_of_next_frame)); 

  ui_begin_build(os_get_client_area_dims(), os_get_mouse_pos(), font);

  ui_push_font_size(pcl->main_font_size);
  ui_push_inner_softness(2);
  ui_push_outer_softness(2);
  
  F32 icon_size = ui_top_font_size() * 3; // Damian: For now this is how it is, TODO

  ui_next_width(ui_p_of_p(1));
  ui_next_height(ui_p_of_p(1));
  UI_Row()
  {
    ui_next_width(ui_fit());
    ui_next_height(ui_grow());
    ui_next_b_color(pcl_color_from_name(PCL_Color_name__main_background, pcl));
    ui_next_padding(ui_top_font_size() * 0.25f);
    ui_next_layout_y();
    ui_next_alignment_x(UI_Alignment_x__center);
    UI_Box* box = ui_box_make_f(UI_Box_flag__has_background|UI_Box_flag__has_padding, "Left box id");
    UI_Parent(box)
    {
      ui_next_width(ui_fit()); 
      ui_next_height(ui_fit()); 
      ui_next_padding(ui_top_font_size());
      ui_next_border(2, pcl_color_from_name(PCL_Color_name__item_selected, pcl));
      ui_next_hover_cursor(OS_Cursor__hand);
      ui_next_corner_r(25);
      ui_next_inner_softness(2);
      ui_next_outer_softness(3);
      UI_Box* home_button = ui_box_make(UI_Box_flag__clickable|UI_Box_flag__has_background|UI_Box_flag__has_borders|UI_Box_flag__has_rounded_corners, Str8FromC("Navigation rail home button"));
      UI_Actions home_button_acts = ui_actions_from_box(home_button);
      
      UI_Parent(home_button)
      {
        ui_image(pcl->icons.home, icon_size, icon_size);
      }
      
      if (home_button_acts.is_clicked)
      {
        pcl_defer_command_to_start_of_next_frame(pcl, PCL_Command__go_to_home);
      }
      if (home_button_acts.is_hovered) {
        ui_box_set_b_color(home_button, pcl_color_from_name(PCL_Color_name__item_selected, pcl));
      }
  
      ui_spacer(ui_grow());
  
      ui_next_width(ui_fit()); 
      ui_next_height(ui_fit()); 
      ui_next_padding(ui_top_font_size());
      ui_next_border(2, pcl_color_from_name(PCL_Color_name__item_selected, pcl));
      ui_next_hover_cursor(OS_Cursor__hand);
      UI_Box* settings_button = ui_box_make(UI_Box_flag__clickable|UI_Box_flag__has_background|UI_Box_flag__has_borders, Str8FromC("Navigation rail setting button"));
      UI_Parent(settings_button)
      {
        ui_image(pcl->icons.settings, icon_size, icon_size);
  
        UI_Actions settings_button_acts = ui_actions_from_box(settings_button);
        if (settings_button_acts.is_clicked) {
          pcl_defer_command_to_start_of_next_frame(pcl, PCL_Command__go_to_settings);
        } 

        if (settings_button_acts.is_hovered) {
          ui_box_set_b_color(settings_button, pcl_color_from_name(PCL_Color_name__item_selected, pcl));
        }
      }
    }
  
    ui_next_width(ui_px(1));
    ui_next_height(ui_grow());
    ui_next_b_color(magenta());
    ui_box_make_f(UI_Box_flag__has_background, "Navigation rail and main page separator");
  
    // Menu box
    ui_next_width(ui_grow());
    ui_next_height(ui_grow());
    ui_next_b_color(pcl_color_from_name(PCL_Color_name__main_background, pcl));
    ui_next_extra_flags(UI_Box_flag__has_background|UI_Box_flag__has_padding);
    ui_next_padding(ui_top_font_size()); // todo: Use a scale here
    UI_Col()
    {
      if (pcl->current_menu == PCL_Menu__settings)
      {
        ui_next_width(ui_grow());
        ui_next_height(ui_rem(1));
        F32 new_font_size = pcl_ui_slider(pcl->main_font_size, rangeF32(4, 64), Str8FromC("Test slider id"));
        
        pcl_defer_command_to_start_of_next_frame(pcl, PCL_Command__new_main_font_size);
        pcl->data_for_commands.new_font_size = new_font_size;
  
        ui_next_font_size(64);
        ui_label_f("Settings menu here");
      }
      else if (pcl->current_menu == PCL_Menu__home)
      {
        Str8 filter_str_for_names = {};

        // TODO: Here you should add a text edit thing
        // ui_next_width(ui_px(250));
        // ui_next_height(ui_fit());
        // ui_next_border(1, red());
        // ui_next_padding(10);
        // ui_next_b_color(white());
        // UI_Parent(ui_box_make(UI_Box_flag__has_background|UI_Box_flag__has_padded_border, {}))
        // {
          // ScratchLoop(scratch, 0, 0)
          // {
            // static U8 text_buffer[255]   = {};
            // static U64 text_buffer_count = 0;
            // static U64 cursor_pos        = 0;
            // static U64 section_pos       = 0;

            // ui_next_font_color(black());
            // UI_Text_op_list text_op_list = ui_text_edit_box(
            //   scratch.arena, 
            //   true, 
            //   ui_grow(), 
            //   text_buffer, 
            //   text_buffer_count, ArrayCount(text_buffer), 
            //   cursor_pos, section_pos,
            //   Str8FromC("Edit box test id for test box")
            // );

            // ui_aply_text_ops(text_op_list, text_buffer, ArrayCount(text_buffer), &text_buffer_count, &cursor_pos, &section_pos, Null, Null);
          
            // filter_str_for_names = str8_manual_view(text_buffer, text_buffer_count);
          // }
        // }

        // ui_spacer(ui_rem(0.25));

        // ui_next_font_size(32);
        // ui_next_font_color(magenta());
        // ui_text_f("Application data");

        // ui_spacer(ui_rem(0.25));

        // DD: Button for adding headers to the table
        ui_next_child_gap(15); ui_next_extra_flags(UI_Box_flag__has_child_gap);
        UI_Row()
        {
          struct {
            Str8 button_text;
            PCL_Table_header_kind header_kind;
          } button_data[] = {
            { Str8FromC("Add name"), PCL_Table_header_kind__name },
            { Str8FromC("Add pid"), PCL_Table_header_kind__pid },
            { Str8FromC("Add ppi"), PCL_Table_header_kind__ppid },
            { Str8FromC("Add startup"), PCL_Table_header_kind__startup_time },
          };
          
          UI_Width(ui_px(50))
          UI_Height(ui_px(50))
          UI_BColor(nice_green())
          UI_Border(1, white())
          UI_Padding(10)
          {
            for EachIndex(i, ArrayCount(button_data))
            {
              if (ui_button(button_data[i].button_text).is_clicked)
              {
                pcl_defer_command_to_start_of_next_frame(pcl, PCL_Command__add_header_to_table);
                pcl->data_for_commands.table_header_kind_for_new_table_header = button_data[i].header_kind;
              }
            }
            
            ui_next_b_color(red());
            if (ui_button_f("Clear table").is_clicked)
            {
              pcl_defer_command_to_start_of_next_frame(pcl, PCL_Command__clear_table);
            }
          }
        }

        ui_spacer(ui_rem(0.25));

        // TODO: There is a bug that the borders are not visible
        // DD: The top table box
        ui_next_width(ui_grow());
        ui_next_height(ui_grow());
        ui_next_padding_ex(20, 20, 10, 10);
        ui_next_padded_border(1, green());
        ui_next_padding(5);
        ui_next_extra_flags(UI_Box_flag__has_padding|UI_Box_flag__has_borders);
        UI_Col()
        {
          // Table + vertical slider on the right of the table
          ui_next_width(ui_grow()); ui_next_height(ui_grow());
          UI_Row()
          {
            B32 is_resizing_headers  = false;
            U64 resizing_header_index = 0;

            // TODO: Move these out, this is here for now
            #define ROW_HEIGHT_SCALER 3 
            #define RESIZER_VISIBLE_WIDTH 1
            
            // DD: Table box itslef
            ui_next_width(ui_grow());
            ui_next_height(ui_grow());
            ui_next_layout_y();
            ui_next_b_color(black());
            UI_Box* table_box = ui_box_make_f(UI_Box_flag__has_background, "table_boxflkdfjlsdk"); 
            UI_Box_data table_box_data = ui_box_data_from_box(table_box);

            // DD: These boxes we will need later, but they are created as children of the table_box
            UI_Box* table_scroll_box_for_rows = ui_box_null();

            if (table_box_data.is_found && pcl->table_data.header_count > 0)
            UI_Parent(table_box)
            {
              U64 n_resizers_per_row = pcl->table_data.header_count - 1;
              F32 space_for_headers  = (F32)(table_box_data.inner_rect.width - (n_resizers_per_row * RESIZER_VISIBLE_WIDTH));
    
              F32 headers_total_flex_value = 0.0f;
              for EachIndex(header_index, pcl->table_data.header_count) {
                headers_total_flex_value += pcl->table_data.headers[header_index].flex_value;
              }
    
              // Floating resizers
              {
                F32 pending_resizer_drag = 0.0;
                
                F32 offset_x = 0.0f;
                for EachIndex(header_index, pcl->table_data.header_count)
                {
                  if (header_index == pcl->table_data.header_count - 1) { continue; }
    
                  PCL_Table_header header = pcl->table_data.headers[header_index];

                  F32 flex_norm = header.flex_value / headers_total_flex_value;
                  offset_x += space_for_headers * flex_norm;
                  offset_x += RESIZER_VISIBLE_WIDTH; 
    
                  #define RESIZER_INVISIBLE_WIDTH 6
                  ui_next_width(ui_px(RESIZER_INVISIBLE_WIDTH));
                  ui_next_height(ui_rem(ROW_HEIGHT_SCALER));
                  ui_next_hover_cursor(OS_Cursor__horizontal_resize);
                  ui_next_floating_fixed_pos_x(offset_x - ((F32)RESIZER_VISIBLE_WIDTH / 2) - ((F32)RESIZER_INVISIBLE_WIDTH / 2));
                  UI_Box* resizer = ui_box_make_f(UI_Box_flag__floating|UI_Box_flag__clickable, "Data table resizer %lld", header_index);

                  UI_Actions resizer_actions = ui_actions_from_box(resizer);
                  if (resizer_actions.is_down)
                  {
                    F32 drag = ui_get_mouse_pos().x - ui_get_prev_mouse_pos().x;
                    is_resizing_headers   = true;
                    resizing_header_index = header_index;
                    pending_resizer_drag  = drag;
                  }
                }
    
                // DD, TODO: This should probably happend in the end sice the rest of the ui depends on these flex values, not so sure about this right now
                if (is_resizing_headers)
                {
                  // TODO: Do this at the end of the frame and see if it does anything

                  F32 old_left_flex  = pcl->table_data.headers[resizing_header_index + 0].flex_value;
                  F32 old_right_flex = pcl->table_data.headers[resizing_header_index + 1].flex_value;
                  
                  F32 old_left_px = (old_left_flex / headers_total_flex_value) * space_for_headers;
    
                  F32 flex_change_on_left = old_left_flex * (pending_resizer_drag / old_left_px);
    
                  F32 new_left_flex  = old_left_flex + flex_change_on_left;
                  F32 new_right_flex = old_right_flex - flex_change_on_left;
    
                  if (new_left_flex > 0.0f && new_right_flex > 0.0f) 
                  {
                    pcl->table_data.headers[resizing_header_index + 0].flex_value = new_left_flex;
                    pcl->table_data.headers[resizing_header_index + 1].flex_value = new_right_flex;
                  }
                }
              }
    
              // TODO: Assert that the flex value from before is the same as now
    
              // DD: Building table headers
              ui_next_size_x(ui_grow());
              ui_next_size_y(ui_rem(ROW_HEIGHT_SCALER));
              ui_next_layout_x();
              UI_Box* table_headers_row_box = ui_box_make_f(0, "Header row for table");

              UI_Parent(table_headers_row_box)
              {
                ScratchLoop(scratch, 0, 0) 
                for (U64 header_index = 0; header_index < pcl->table_data.header_count; header_index += 1, reset_scratch(&scratch))
                {
                  PCL_Table_header header = pcl->table_data.headers[header_index];
                  F32 flex_norm           = header.flex_value / headers_total_flex_value;
                  
                  ui_next_width(ui_px(flex_norm * space_for_headers));
                  ui_next_height(ui_grow());
                  
                  // TODO: Sort the headers here by generation to then be able to have stable ids
                  Str8 id = str8_fmt(scratch.arena, "Table header %lld", header_index);
                  UI_Actions header_actions = pcl_ui_table_header(id, header, pcl);

                  Str8 header_context_menu_id = str8_fmt(scratch.arena, "Table header context menu id _ %lld", header.generation);
                  if (header_actions.is_right_clicked)
                  {
                    ui_set_context_menu_key(header_context_menu_id, ui_get_mouse_pos());
                  }

                  UI_ContextMenu(header_context_menu_id)
                  {
                    ui_next_width(ui_px(100));
                    ui_next_height(ui_px(100));
                    ui_next_b_color(black()); // TODO: Change this
                    // ui_next_inner_softness(2);
                    // ui_next_outer_softness(2);
                    ui_next_corner_r(7);
                    ui_next_border(5, golden());
                    // ui_next_corner_r(0.25f * ui_top_font_size());
                    // ui_next_border(ui_top_font_size() * 0.25f, red());
                    ui_next_padding(ui_top_font_size() * 0.35f);
                    ui_next_child_gap(ui_top_font_size() * 0.1f);
                    UI_Box* button_list_box = ui_box_make(
                      UI_Box_flag__has_padded_border
                      |UI_Box_flag__has_borders
                      |UI_Box_flag__has_background
                      |UI_Box_flag__has_child_gap
                      |UI_Box_flag__has_rounded_corners, 
                      {}
                    );
                    
                    struct {
                      Str8 button_text;
                      PCL_Command command_to_defere_on_click;
                    } data_for_button_interaction[] = {
                      { Str8FromC("Remove header"), PCL_Command__remove_currenly_selected_header },
                      { Str8FromC("Add PID Column"), PCL_Command__add_PID_header_after_the_current_selected_header },
                    };

                    UI_Parent(button_list_box)
                      for EachIndex(i, ArrayCount(data_for_button_interaction))
                        UI_CornerR(0.15f * ui_top_font_size())
                        UI_Padding(ui_top_font_size() * 0.15f)
                    {
                      UI_Actions button_actions = ui_button(data_for_button_interaction[i].button_text);
                      if (button_actions.is_hovered) { ui_box_set_b_color(button_actions.box, nice_blue()); }
                      if (button_actions.is_clicked) { 
                        pcl_defer_command_to_start_of_next_frame(pcl, data_for_button_interaction[i].command_to_defere_on_click); 
                        ui_reset_context_menu();
                      }
                    }
                  }

                  if (header_index != pcl->table_data.header_count - 1)
                  {
                    if (is_resizing_headers && header_index == resizing_header_index) { ui_next_b_color(red()); } else { ui_next_b_color(nice_blue()); }
                    ui_next_width(ui_px(RESIZER_VISIBLE_WIDTH));
                    ui_next_height(ui_grow());
                    UI_Box* visible_resizer_box = ui_box_make(UI_Box_flag__has_background, {});
                  }

                }
              }
    
              // Horizontal separator between the headers and the table body or rows
              ui_next_width(ui_grow());
              ui_next_height(ui_px(1));
              ui_next_b_color(orange());
              UI_Box* separator_between_header_row_and_data_rows = ui_box_make(UI_Box_flag__has_background, {});

              ui_next_width(ui_grow());
              ui_next_height(ui_grow());
              ui_next_layout_y();
              table_scroll_box_for_rows = ui_box_make_f(UI_Box_flag__clip, "Table scroll box for rows");

              UI_Parent(table_scroll_box_for_rows)
              {
                // Getting data to make a virtual list for the table data
                F32 space_before_first_visible_row = 0.0f;
                F32 space_for_visible_rows         = 0.0f;
                F32 space_after_last_visible_row   = 0.0f;
                //
                U64 first_visible_row_index = 0;
                U64 last_visible_row_index  = 0;
                //
                F32 row_size = 0.0f;
                F32 space_between_rows = 2;
                {
                  U64 n_rows        = pcl->gathered_process_data_this_frame.count;
                  row_size          = ui_top_font_size() * ROW_HEIGHT_SCALER;
                  F32 offset        = -ui_box_clip_offset(table_scroll_box_for_rows).y;
                  F32 vp            = ui_box_clip_data_from_box(table_scroll_box_for_rows).viewport_dims.y;
                  if (offset < 0.0f) { offset = 0.0f; }
  
                  first_visible_row_index = (U64)(offset / (row_size + space_between_rows));
                  last_visible_row_index  = (U64)((offset + vp + 2*(row_size + space_between_rows)) / (row_size + space_between_rows));
                  if (first_visible_row_index > 0) { first_visible_row_index -= 1; }
                  clamp_u64_inplace(&first_visible_row_index, 0, n_rows);
                  clamp_u64_inplace(&last_visible_row_index, 0, n_rows);
  
                  space_before_first_visible_row = (row_size + space_between_rows) * first_visible_row_index;
                  space_for_visible_rows         = (row_size + space_between_rows) * (last_visible_row_index - first_visible_row_index);
                  space_after_last_visible_row   = (row_size + space_between_rows) * (n_rows - last_visible_row_index);
                  
                  {
                    U64 _n_rows = first_visible_row_index + (last_visible_row_index - first_visible_row_index) + (pcl->gathered_process_data_this_frame.count - last_visible_row_index);
                    Assert(_n_rows == pcl->gathered_process_data_this_frame.count, "Your count is invalid buddy"); 
                  }
                }

                ui_next_width(ui_grow());
                ui_next_height(ui_px(space_before_first_visible_row));
                UI_Box* first_space_filler = ui_box_make(0, {});

                for (
                  U64 process_data_index = first_visible_row_index; 
                  process_data_index < last_visible_row_index; 
                  process_data_index += 1
                ) {
                  WindowInfo* process_data = &pcl->gathered_process_data_this_frame.v[process_data_index];

                  ui_next_width(ui_grow());
                  ui_next_height(ui_px(row_size));
                  ui_next_layout_x();
                  ui_next_padded_border(1, transparent());
                  UI_Box* row_box = ui_box_make_f(UI_Box_flag__has_background|UI_Box_flag__has_padding|UI_Box_flag__has_borders|UI_Box_flag__clickable, "Table row box %lld", process_data_index);
                  UI_Actions row_actions = ui_actions_from_box(row_box);
  
                  if (row_actions.is_hovered)
                  {
                    ui_scroll_box_with_wheel(table_scroll_box_for_rows, 15);
                  }

                  if (row_actions.went_down)
                  {
                    pcl_defer_command_to_start_of_next_frame(pcl, PCL_Command__select_row);
                    pcl->data_for_commands.process_at_row_to_select_pid = process_data->pid;
                  }
                  
                  B32 is_row_selected = (pcl->selected_row_data.is_selected && pcl->selected_row_data.pid == process_data->pid);
                  
                  V4F32 row_b_color = pcl_color_from_name(PCL_Color_name__main_background, pcl);
                  {
                    if (is_row_selected) { row_b_color = lerp_v4f32(row_b_color, red(), 0.25f); }
                    else if (row_actions.is_hovered) { row_b_color = lerp_v4f32(row_b_color, red(), 0.15f); }
                  }
    
                  ui_box_set_b_color(row_box, row_b_color);
                  if (is_row_selected) { ui_box_set_border(row_box, v4f32_all(1), red()); }
  
                  UI_Parent(row_box)
                  {
                    for EachIndex(header_index, pcl->table_data.header_count)
                    {
                      F32 flex_norm = pcl->table_data.headers[header_index].flex_value / headers_total_flex_value;
      
                      PCL_Table_header header = pcl->table_data.headers[header_index];
  
                      ui_next_width(ui_px(flex_norm * space_for_headers));
                      ui_next_height(ui_grow()); 
                      UI_Parent(ui_box_make(UI_Box_flag__dont_draw_overflow, {})) // 
                      {
                        switch(header.kind)
                        {
                          default: { InvalidCodePath(); } break;
      
                          case PCL_Table_header_kind__NONE:
                          {
                            // DD: Empty
                          } break;
  
                          case PCL_Table_header_kind__name:
                          {
                            ui_text_f("--Name--");
                          } break;

                          case PCL_Table_header_kind__pid:
                          {
                            ui_text_f("%d", process_data->pid);
                          } break;
  
                          case PCL_Table_header_kind__ppid:
                          {
                            ui_text_f("%d", process_data->ppid);
                          } break;
  
                          case PCL_Table_header_kind__startup_time:
                          {
                            // ui_text(process_data->create_time);
                          } break;
                        }
                      }
                    }
                  }
                
                  // DD: Little spacer between the rows
                  // if (process_data->next != 0)
                  if (process_data_index < pcl->gathered_process_data_this_frame.count - 1)
                  {
                    ui_spacer(ui_px(space_between_rows));
                  }
                }

                ui_next_width(ui_grow());
                ui_next_height(ui_px(space_after_last_visible_row));
                UI_Box* last_space_filler = ui_box_make(0, {});
              }
            }

            ui_spacer(ui_rem(0.25f));

            { // DD: Y Scroll bar for the table 
              UI_Box_clip_data table_clip_data = ui_box_clip_data_from_box(table_scroll_box_for_rows);
              F32 out_new_scroll = 0.0f;
              B32 is_new_offset  = false;
              pcl_scroll_bar(
                ui_px(50), /*ui_grow()*/ ui_px(250), Axis2__y, Str8FromC("Scroll bar for table"),
                table_clip_data.viewport_dims.y, table_clip_data.content_dims.y, -ui_box_clip_offset(table_scroll_box_for_rows).y,
                &out_new_scroll, &is_new_offset
              );
              if (is_new_offset) 
              {
                // TODO: DO we need defered scroll here 
                table_scroll_box_for_rows->defered_clip_offset.is_present = true; 
                table_scroll_box_for_rows->defered_clip_offset.offset.y = -out_new_scroll;
              }
            }

            ui_spacer(ui_px(10));

            ui_next_font_size(15);
            ui_text_f("%lld", pcl->gathered_process_data_this_frame.count);
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
    if (pcl->is_command_window_open)
    {
      UI_Parent(ui_get_root())
      {
        ui_next_width(ui_grow());
        ui_next_height(ui_grow());
        ui_next_alignment_x(UI_Alignment_x__center);
        ui_next_alignment_y(UI_Alignment_y__center);
        ui_next_extra_flags(UI_Box_flag__floating);
        UI_Parent(ui_box_make(0, {}))
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
              UI_Parent(ui_box_make(0, {}))
              {
                UI_FontSize(ui_top_font().size)
                {
                  Scratch scratch = get_scratch(0, 0);
    
                  // static U8 text_buffer[32] = {};
                  // static U64 text_buffer_size = {};
                  // static U64 cursor_pos = {};
                  // static U64 section_start_pos = {};

                  // ui_next_b_color(blue());
                  // ui_next_extra_flags(UI_Box_flag__has_background);
                  // UI_Text_op_list op_list = {};
                  // UI_Col()
                  // {
                  //   UI_Text_op_list op_list_ = ui_text_edit_box(scratch.arena, true, ui_px(300), text_buffer, text_buffer_size, ArrayCount(text_buffer), cursor_pos, section_start_pos, Str8FromC("Test text edit field"));
                  //   op_list = op_list_;
                  // }

                  // ui_aply_text_ops(op_list, text_buffer, ArrayCount(text_buffer), &text_buffer_size, &cursor_pos, &section_start_pos, Null, Null);
    
                  end_scratch(&scratch);
                }
              }

              // todo: Here just spawn each of the command thing to show the user to use
              
              // ui_next_width(ui_grow());
              // ui_next_height(ui_grow());
              // ui_next_b_color(dark_back_color);
              // UI_Box* command_scroll_list = ui_box_make(UI_Box_flag__has_background|UI_Box_flag__clip, {});
              // UI_Parent(command_scroll_list)
              // { 
              //   for EachEnumRange(command, PCL_Command, PCL_Command__go_to_settings, PCL_Command__COUNT)
              //   {
              //     UI_Col()
              //     {
              //       ui_label(pcl_command_name_for_user[command]);
              //       // ui_label_f("Some text in smaller font that tells the user in detail about the command");
              //     }
              //   }
              // }

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

  if (pcl->show_debug_data)
  {
    ui_next_width(ui_fit());
    ui_next_height(ui_fit());
    ui_next_b_color(black());
    UI_Box* debug_floating_box = ui_box_make(UI_Box_flag__floating|UI_Box_flag__has_background, {});
    
    UI_Parent(debug_floating_box)
      UI_FontSize(24)
    {
      ui_next_font_color(nice_blue());
      ui_text_f("DEBUG floating box: ");

      ui_spacer(ui_px(15));

      ui_text_f("Clay->layoutElementsHashMapInternal.length: %d", Clay_GetCurrentContext()->layoutElementsHashMapInternal.length);
      ui_text_f("Clay->layoutElementsHashMapFreeList.length: %d", Clay_GetCurrentContext()->layoutElementsHashMapFreeList.length);

      ui_spacer(ui_px(15));

      U64 boxed_in_table = 0;
      for EachIndex(i, ArrayCount(ui_get_state()->hash_table_buckets)) {
        boxed_in_table += ui_get_state()->hash_table_buckets[i].count;
      }
      ui_text_f("Count of boxed in the hash table: %lld", boxed_in_table);

      ui_spacer(ui_px(15));

      ui_text_f("UI Boxes in use right now: %lld", ui_get_state()->last_build_box_count);
      ui_text_f("UI Free boxes count: %lld",       ui_get_state()->count_of_free_boxes);
      ui_text_f("UI Generation: %lld",             ui_get_state()->build_generation);

      ui_spacer(ui_px(15));

      // ui_text_f("Times Scissor rect got attempted to be pushed this frame: %lld", draw_debug_data.times_scissor_rect_was_attempted_to_be_pushed);
      // ui_text_f("Times Scissor rect got pushed this frame: %lld", draw_debug_data.times_scissor_rect_got_pushed);

      ui_spacer(ui_px(15));

      // ui_text_f("spall_times_used_this_frame: %lld", times_spall_got_used_last_frame);

      ui_spacer(ui_px(15));

      if (0) {}
      else if (prev_frame_fps < 30)   { ui_next_font_color(red());        }
      else if (prev_frame_fps < 60)   { ui_next_font_color(orange());     }
      else if (prev_frame_fps < 120)  { ui_next_font_color(yellow());     }
      else if (prev_frame_fps < 165)  { ui_next_font_color(green());      }
      else if (prev_frame_fps < 240)  { ui_next_font_color(nice_green()); }
      else if (prev_frame_fps < 360)  { ui_next_font_color(teal());       }
      else if (prev_frame_fps < 500)  { ui_next_font_color(blue());       }
      else if (prev_frame_fps < 700)  { ui_next_font_color(nice_blue());  }
      else if (prev_frame_fps < 850)  { ui_next_font_color(magenta());    }
      else if (prev_frame_fps < 1000) { ui_next_font_color(golden());     }
      else                            { ui_next_font_color(pink());       }
      ui_text_f("FPS: %lld", prev_frame_fps);

    }
  }

  ui_end_build();

  // TODO: Find out why these crash main thread, thought the values you are supposed to be working with here are the dll specific once, maybe the draw layer that crashed is not specific to the dll, look into it
  // os_set_state(0);
  // set_thread_context(0);
  // r_set_state(0);
  // fp_set_state(0);
  // d_set_state(0);
  // ui_set_state(0);

  ProfEndGroup();
}

///////////////////////////////////////////////////////////
// - UI
//
V4F32 pcl_color_from_name(PCL_Color_name color_name, PCL_State* pcl)
{
  if (color_name == PCL_Color_name__NONE || color_name == PCL_Color_name__COUNT) {
    Assert(0);
    return magenta();
  }
  V4F32 color = pcl->color_values_for_names[color_name];
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
  UI_Box* slider_top_box = ui_box_make(UI_Box_flag__has_padding|UI_Box_flag__has_borders, id);
  UI_Parent(slider_top_box)
  {
    F32 value_normalised = (value - range_for_value.min) / (range_for_value.max - range_for_value.min);
    ui_next_width(ui_p_of_p(value_normalised));
    ui_next_height(ui_grow());
    ui_next_b_color(blue());
    ui_next_padding(5);
    UI_Parent(ui_box_make(UI_Box_flag__has_background|UI_Box_flag__has_padding, {}))
    {
      ui_next_width(ui_grow());
      ui_next_height(ui_grow());
      ui_next_b_color(white());
      ui_box_make(UI_Box_flag__has_background, {});
    }
  }

  F32 new_value = value;
  UI_Actions slider_acts = ui_actions_from_box(slider_top_box);
  if (slider_acts.is_down)
  {
    // BP;
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

void pcl_scroll_bar(UI_Size size_x, UI_Size size_y, Axis2 scroll_axis, Str8 scroll_bar_id, F32 outer_vp_size, F32 outer_content_size, F32 outer_vp_offset, F32* out_new_scroll, B32* is_new_offset)
{
  Assert(size_x.kind != UI_Size_kind__fit);
  Assert(size_y.kind != UI_Size_kind__fit);

  // DD: I dont thing that it metters much weather we make the ui for the prev frame data or update the data
  //     and then make the scroll bar based on that data, so i will just use the later version.

  Scratch scratch = get_scratch(0, 0);
  
  Str8 before_thumb_box_id = str8_fmt(scratch.arena, "%.*s__before_thumb_box", Str8FmtArg(scroll_bar_id)); 
  Str8 thumb_id            = str8_fmt(scratch.arena, "%.*s__thumb",            Str8FmtArg(scroll_bar_id)); 
  Str8 after_thumb_box_id  = str8_fmt(scratch.arena, "%.*s__after_thumb_box",  Str8FmtArg(scroll_bar_id)); 

  UI_Actions before_thumb_box_actions = ui_actions_from_id(before_thumb_box_id);
  UI_Actions thumb_actions            = ui_actions_from_id(thumb_id);
  UI_Actions after_thumb_box_actions  = ui_actions_from_id(after_thumb_box_id);

  UI_Box_data scroll_bar_data       = ui_box_data_from_id(scroll_bar_id);
  UI_Box_data before_thumb_box_data = ui_box_data_from_id(before_thumb_box_id);
  UI_Box_data thumb_data            = ui_box_data_from_id(thumb_id);
  UI_Box_data after_thumb_box_data  = ui_box_data_from_id(after_thumb_box_id);
  
  F32 thumb_offset     = 0.0f;
  F32 max_thumb_offset = 0.0f;
  F32 thumb_size       = 0.0f;

  if ( scroll_bar_data.is_found 
    && thumb_data.is_found 
    && before_thumb_box_data.is_found 
    && after_thumb_box_data.is_found
  ) {
    if (outer_vp_size == 0.0f || outer_content_size == 0.0f)
    {
      // DD: Just making the thumb take the whole scroll bar in that case
      thumb_size = scroll_bar_data.inner_rect.dims.v[scroll_axis];
    }
    else 
    {
      F32 thumb_min_size          = 20;
      F32 scroll_bar_scroll_space = scroll_bar_data.rect.dims.v[scroll_axis];
      F32 inner_space             = scroll_bar_data.inner_rect.dims.v[scroll_axis];
      F32 max_thumb_size          = inner_space;

      thumb_size = (outer_vp_size / outer_content_size) * max_thumb_size;
      if (thumb_size > inner_space) { thumb_size = inner_space; }
      if (thumb_size < thumb_min_size) { thumb_size = thumb_min_size; }
      
      F32 max_vp_offset = outer_content_size - outer_vp_size;
      max_thumb_offset  = inner_space - thumb_size;
      
      thumb_offset = (outer_vp_offset / max_vp_offset) * max_thumb_offset;
  
      struct Drag_data {
        F32 inside_thumb_position_at_drag_start;
      };
  
      B32 dragged = false;
      if (0) {}
      else // DD: Logic for thumb 
      if (thumb_actions.went_down)
      {
        UI_Box_data data = ui_box_data_from_id(thumb_id);
        Data_buffer* buffer = ui_box_drag_buffer_by_id(scroll_bar_id);
        // TODO: This drag api is not the best, but i dont know what i dont like about it
        if (buffer->count == 0)
        {
          buffer = ui_box_drag_buffer_alloc_by_id(scroll_bar_id, sizeof(Drag_data));
        }
        Drag_data* drag_data = (Drag_data*)buffer->data;
        drag_data->inside_thumb_position_at_drag_start = ui_get_mouse_pos().v[scroll_axis] - data.inner_rect.origin.v[scroll_axis];
      }
      else // DD: Logic for the space around the thumb
      if (before_thumb_box_actions.went_down || after_thumb_box_actions.went_down)
      {
        F32 mouse_relative_to_slide_zone        = ui_get_mouse_pos().v[scroll_axis] - scroll_bar_data.inner_rect.origin.v[scroll_axis];
        F32 thumb_center_relative_to_slide_zone = rect_center(thumb_data.rect).v[scroll_axis] - scroll_bar_data.inner_rect.origin.v[scroll_axis];
        
        F32 offset_to_add_to_thumb_to_have_thumb_center_at_mouse = mouse_relative_to_slide_zone - thumb_center_relative_to_slide_zone; 
  
        F32 test_new_offset         = thumb_offset + offset_to_add_to_thumb_to_have_thumb_center_at_mouse;
        F32 test_new_offset_clamped = clamp_f32(test_new_offset, 0.0f, max_thumb_offset);
        
        F32 diff = test_new_offset - test_new_offset_clamped;
        F32 offset_we_can_add = offset_to_add_to_thumb_to_have_thumb_center_at_mouse - diff;
  
        thumb_offset += offset_we_can_add;
  
        F32 new_thumb_offset_inside_slider_zone = thumb_offset;
    
        // TODO: This drag api is not the best, but i dont know what i dont like about it
        Data_buffer* buffer = ui_box_drag_buffer_by_id(scroll_bar_id);
        if (buffer->count == 0)
        {
          buffer = ui_box_drag_buffer_alloc_by_id(scroll_bar_id, sizeof(Drag_data));
        }
        Drag_data* drag_data = (Drag_data*)buffer->data;
        drag_data->inside_thumb_position_at_drag_start = mouse_relative_to_slide_zone - new_thumb_offset_inside_slider_zone;
  
        dragged = true;
      }
      else // DD: Main dragging code
      if (thumb_actions.is_down || before_thumb_box_actions.is_down || after_thumb_box_actions.is_down)
      {
        Data_buffer* buffer = ui_box_drag_buffer_by_id(scroll_bar_id);
        Drag_data* drag_data = (Drag_data*)buffer->data;
  
        dragged = true;
        UI_Box_data data         = ui_box_data_from_id(thumb_id);
        F32 inside_thumb_pos_now = ui_get_mouse_pos().v[scroll_axis] - data.inner_rect.origin.v[scroll_axis];
        F32 diff                 = inside_thumb_pos_now - drag_data->inside_thumb_position_at_drag_start;
        thumb_offset += diff;
      }
      else // DD: Done dragging 
      if (thumb_actions.went_up || before_thumb_box_actions.went_up || after_thumb_box_actions.went_up)
      {
        ui_box_drag_buffer_release_by_id(scroll_bar_id);
      }
  
      thumb_offset = clamp_f32(thumb_offset, 0.0f, max_thumb_offset);
  
      if (dragged)
      {
        *is_new_offset  = true;
        *out_new_scroll = (thumb_offset / max_thumb_offset) * max_vp_offset;
      }
    }
  }

  ui_next_width(size_x);
  ui_next_height(size_y);
  ui_next_b_color(nice_green());
  ui_next_padding(4);
  ui_next_layout(scroll_axis);
  UI_Box* scroll_bar = ui_box_make(
    UI_Box_flag__has_background|
    UI_Box_flag__has_padding, 
    scroll_bar_id
  );

  UI_Parent(scroll_bar)
  {
    ui_next_size_axis(scroll_axis, ui_px(thumb_offset));
    ui_next_size_axis(axis2_other(scroll_axis), ui_grow()); 
    UI_Box* before_thumb_box = ui_box_make(UI_Box_flag__clickable, before_thumb_box_id);

    ui_next_size_axis(scroll_axis, ui_px(thumb_size));
    ui_next_size_axis(axis2_other(scroll_axis), ui_grow()); 
    ui_next_b_color(magenta());
    UI_Box* thumb = ui_box_make(UI_Box_flag__clickable|UI_Box_flag__has_background, thumb_id);

    ui_next_size_axis(scroll_axis, ui_px(max_thumb_offset - thumb_offset));
    ui_next_size_axis(axis2_other(scroll_axis), ui_grow()); 
    UI_Box* after_thumb_box = ui_box_make(UI_Box_flag__clickable, after_thumb_box_id);
  }
  
  end_scratch(&scratch);
}

UI_Actions pcl_ui_table_header(Str8 id, PCL_Table_header header, PCL_State* pcl)
{
  ui_next_layout_x();
  UI_Box* box = ui_box_make(
    UI_Box_flag__dont_draw_overflow|
    UI_Box_flag__has_background|
    UI_Box_flag__clickable, 
    id
  );
  V4F32 b_color = pcl_color_from_name(PCL_Color_name__main_background, pcl);
  if (header.is_used_for_sorting) { b_color = brown(); }
  
  UI_Actions actions = ui_actions_from_box(box); 
  if (actions.is_hovered) { b_color = lerp_v4f32(b_color, white(), 0.15f); }

  ui_box_set_b_color(box, b_color);

  UI_Parent(box)
  {
    Str8 text_for_header_kind = {};
    if (0) {}
    else if (header.kind == PCL_Table_header_kind__NONE)         { text_for_header_kind = Str8FromC(""); }
    else if (header.kind == PCL_Table_header_kind__name)         { text_for_header_kind = Str8FromC("Name"); }
    else if (header.kind == PCL_Table_header_kind__pid)          { text_for_header_kind = Str8FromC("PID"); }
    else if (header.kind == PCL_Table_header_kind__ppid)         { text_for_header_kind = Str8FromC("PPID"); }
    else if (header.kind == PCL_Table_header_kind__startup_time) { text_for_header_kind = Str8FromC("Startup"); }

    ui_next_width(ui_grow());
    ui_next_height(ui_grow());
    ui_text_ellipsed(text_for_header_kind);
    
    if (header.is_used_for_sorting)
    {
      R_Handle arrow_icon = (header.sort_small_to_big ? pcl->icons.arrow_up : pcl->icons.arrow_down);
      ui_image(arrow_icon, ui_top_font_size(), ui_top_font_size());
    }

  }

  return actions;
}


#endif
