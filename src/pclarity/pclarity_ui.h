#ifndef PCLARITY_UI_H
#define PCLARITY_UI_H

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
  F32 main_font_size;

  // Data that might be used for commands at the start of the next frame
  F32 new_font_size;

  Arena* frame_arena;
  PCL_Command_list defered_commands_to_start_of_next_frame;
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
    ui_next_height(ui_p_of_p(1));
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
    ui_next_height(ui_p_of_p(1));
    ui_next_b_color(magenta());
    ui_box_make_f("Navigation rail and main page separator", UI_Box_flag__has_background);
  
    if (PCL->current_menu == PCL_Menu__settings)
    {
      ui_next_padding(50);
      ui_next_extra_flags(UI_Box_flag__has_padding);
      ui_next_width(ui_px(500));
      ui_next_height(ui_px(500));
      UI_Col()
      {
        ui_next_width(ui_grow());
        ui_next_height(ui_rem(1));
        F32 new_font_size = pcl_ui_slider(PCL->main_font_size, rangeF32(4, 64), Str8FromC("Test slider id"));
        pcl_defer_command_to_start_of_next_frame(PCL, PCL_Command__new_main_font_size);
        PCL->new_font_size = new_font_size;
      }

      ui_next_font_size(64);
      ui_label_f("Settings menu here");
    }
    else if (PCL->current_menu == PCL_Menu__home)
    {
       ui_next_width(ui_grow());
       ui_next_height(ui_grow());
       ui_next_padding_diff(20, 20, 10, 10);
       ui_next_extra_flags(UI_Box_flag__has_padding);
       UI_Col()
       {
         // todo: have a text edit field in there
   
         // TODO: Have this width here not be static all the time
         ui_next_width(ui_px(300));
         ui_next_height(ui_fit());
         ui_next_padded_border(2, pcl_color_from_name(PCL_Color_name__item_selected));
         ui_next_extra_flags(UI_Box_flag__has_borders);
         ui_next_alignment_y(UI_Alignment_y__center);
         UI_Row()
         {
           ui_image(pcl_icon_magnifying_glass, icon_size, icon_size);
           
           ui_spacer(ui_px(15)); // TODO: This shoud be relative to font size or some like that 
           
           // TODO: Here will be the seach bar later
           ui_label_f("Search programs...");
         }
         
         ui_spacer(ui_px(20));
   
         ui_next_font_size(32);
         ui_next_text_color(white());
         ui_label_f("App statistics");
   
   
   
         // TODO: Here will be the thing that displayed the whole list of apps and other data related to it
       }
    }
    else {
      InvalidCodePath();
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

///////////////////////////////////////////////////////////
// - Extracting table api like you saw casey do 
//
struct UI_Table_config {
  F32 flex_values_for_headers[64]; // TODO: Dont have it be capped like that
  U64 flex_values_for_headers_count;

  F32 row_size_in_pixels;

  V4F32 border_color;
  F32 border_around_width;
};

struct UI_Table_row {
  UI_Box** row_entries;
  U64 count;
};

void table_ui_step_1(
  // Input
  UI_Table_config* table_conf, 
  UI_Size width_size, 
  UI_Size height_size,
  U64 n_rows, 
  
  // Output
  Arena* arena_for_out_boxes, 
  UI_Box*** out_array_of_header_boxes, U64* out_array_of_header_boxes_size,
  UI_Table_row** out_array_of_rows, U64* out_array_of_rows_size
) {
  // Table ui build
  B32 did_resizer_get_dragged = false;
  U64 column_index_whos_resizer_got_dragged = 0;

  // TODO: Do resizing here first
  // "Resizer %lld"
  
  UI_Actions resizer_actions = ui_actions_from_box(resizer);
  if (resizer_actions.is_down)
  {
    did_resizer_get_dragged = true;
    column_index_whos_resizer_got_dragged = header_index;
  }


  //
  ui_next_width(width_size);
  ui_next_height(height_size);
  ui_next_padded_border(table_conf->border_around_width, table_conf->border_color);
  ui_next_extra_flags(UI_Box_flag__has_borders|UI_Box_flag__has_padding|UI_Box_flag__clip);
  UI_Col()
  {
    U64 number_of_headers = table_conf->flex_values_for_headers_count;
    UI_Box** arr_of_header_boxes = ArenaPushArr(arena_for_out_boxes, UI_Box*, number_of_headers);

    // Header ui
    ui_next_width(ui_grow());
    UI_Row()
    {
      F32 total_flex_value = 0.0f;
      for EachIndex(header_index, table_conf->flex_values_for_headers_count)
      {
        total_flex_value += table_conf->flex_values_for_headers[header_index];
      }

      // TODO: This might be customisable or some like that, not sure
      Assert(table_conf->flex_values_for_headers_count > 0);
      F32 px_size_for_resizer = 3.0f;
      F32 extra_size_for_resizers = px_size_for_resizer * (table_conf->flex_values_for_headers_count - 1);
      total_flex_value += extra_size_for_resizers;

      for EachIndex(header_index, table_conf->flex_values_for_headers_count)
      {
        F32 flex_percentage = table_conf->flex_values_for_headers[header_index] / total_flex_value;

        // == OLD CODE ==
        ui_next_width(ui_p_of_p(flex_percentage));
        ui_next_height(ui_px(table_conf->row_size_in_pixels)); 
        ui_next_layout_x();
        UI_Box* header_box = ui_box_make_f("Table header %lld", 0, header_index);
        UI_Parent(header_box)
        {
          ui_next_width(ui_grow());
          ui_next_height(ui_grow());
          ui_next_padded_border(table_conf->border_around_width, table_conf->border_color);
          ui_next_alignment_x(UI_Alignment_x__center);
          UI_Box* box_for_user = ui_box_make({}, UI_Box_flag__has_borders|UI_Box_flag__has_padding);

          arr_of_header_boxes[header_index] = box_for_user;
        }

        if (header_index != (table_conf->flex_values_for_headers_count - 1))
        {
          ui_next_width(ui_px(px_size_for_resizer));
          ui_next_height(ui_px(table_conf->row_size_in_pixels));
          ui_next_b_color(magenta());
          ui_next_hover_cursor(OS_Cursor__horizontal_resize);
          UI_Box* resizer = ui_box_make_f("Resizer %lld", UI_Box_flag__has_background, header_index);

          UI_Actions resizer_actions = ui_actions_from_box(resizer);
          if (resizer_actions.is_down)
          {
            did_resizer_get_dragged = true;
            column_index_whos_resizer_got_dragged = header_index;
          }

        }

      }
    }

    *out_array_of_header_boxes = arr_of_header_boxes; // This here is kind of code repetition
    *out_array_of_header_boxes_size = number_of_headers; // This here is kind of code repetition

    // todo: These allocation here might be made better and more in order, but thats fine for now
    UI_Table_row* array_of_rows = ArenaPushArr(arena_for_out_boxes, UI_Table_row, n_rows);

    for EachIndex(row_index, n_rows)
    {
      UI_Table_row* row = array_of_rows + row_index;
      row->count = table_conf->flex_values_for_headers_count;
      row->row_entries = ArenaPushArr(arena_for_out_boxes, UI_Box*, row->count);
    }

    // Rows ui
    for EachIndex(row_index, n_rows)
    {
      UI_Table_row* row = array_of_rows + row_index;

      ui_next_width(ui_p_of_p(1));
      ui_next_height(ui_px(table_conf->row_size_in_pixels));
      UI_Row()
      {
        // TODO: This is already above, this is duplicated code
        F32 total_flex_value = 0.0f;
        for EachIndex(header_index, table_conf->flex_values_for_headers_count)
        {
          total_flex_value += table_conf->flex_values_for_headers[header_index];
        }

        for EachIndex(header_index, table_conf->flex_values_for_headers_count)
        {
          F32 flex_percentage = table_conf->flex_values_for_headers[header_index] / total_flex_value;

          ui_next_width(ui_p_of_p(flex_percentage));
          ui_next_height(ui_px(table_conf->row_size_in_pixels));
          ui_next_layout_x();
          ui_next_padded_border(table_conf->border_around_width, table_conf->border_color);
          UI_Box* row_entry_box = ui_box_make({}, UI_Box_flag__clip|UI_Box_flag__has_borders|UI_Box_flag__has_padding);
          
          row->row_entries[header_index] = row_entry_box;
        }
      }
    }
    
    *out_array_of_rows = array_of_rows;
    *out_array_of_rows_size = n_rows;
  }

  // Table resizing
  if (did_resizer_get_dragged) ScratchLoop(scratch, 0, 0)
  {
    B32 all_headers_found = true;
    U64 header_count = table_conf->flex_values_for_headers_count;
    Rect* rects_for_each_header = ArenaPushArr(scratch.arena, Rect, header_count);
    
    for EachIndex(header_index, header_count)
    {
      Str8 header_id = str8_fmt(scratch.arena, "Table header %lld", header_index);
      UI_Box_data header_box_data = ui_box_data_from_id(header_id);
      if (!header_box_data.is_found) {
        all_headers_found = false;
      } else {
        rects_for_each_header[header_index] = header_box_data.rect;
      }
    }

    // Damian: Updating the sizer for headers
    if (all_headers_found)
    {
      F32 drag = ui_get_mouse_pos().x - ui_get_prev_mouse_pos().x; // TODO: This should use data from the actions and not just this, but for now its fine
      
      OutputDebugStringF("Drag: %f \n", drag);
      if (drag != 0.0f)
      {
        // BP;
        F32 drag_norm = drag / rects_for_each_header[column_index_whos_resizer_got_dragged].width;
        table_conf->flex_values_for_headers[column_index_whos_resizer_got_dragged] += (table_conf->flex_values_for_headers[column_index_whos_resizer_got_dragged] * drag_norm);
        table_conf->flex_values_for_headers[column_index_whos_resizer_got_dragged + 1] -= (table_conf->flex_values_for_headers[column_index_whos_resizer_got_dragged + 1] * drag_norm);

        // rects_for_each_header[column_index_whos_resizer_got_dragged].width += drag;
        // rects_for_each_header[column_index_whos_resizer_got_dragged + 1].width -= drag;
  
        // Storing back the new flex values
        // for EachIndex(header_index, header_count)
        // {
        //   table_conf->flex_values_for_headers[header_index] = rects_for_each_header[header_index].width;
        // }
      }

    }
  }
}

// TODO:
// Seems like the resizers should not be part of the box that is then has the box that we give out to the user
// it would make more sense to just have it be in between the user boxes.
void table_do_ui_build(UI_Table_config* table_conf, FP_Font font, F32 width, F32 height)
{
  static struct {
    R_Handle texture;
    Str8 name;
    Str8 usage_word;
  } process_data_arr[3] = {};

  process_data_arr[0] = { r_load_texture_from_file(Str8FromC("../data/icons/house.png")), Str8FromC("Discord"), Str8FromC("Low") };
  process_data_arr[1] = { r_load_texture_from_file(Str8FromC("../data/icons/house.png")), Str8FromC("Telegram"), Str8FromC("Low") };
  process_data_arr[2] = { r_load_texture_from_file(Str8FromC("../data/icons/house.png")), Str8FromC("Minecraft"), Str8FromC("Low") };

  ui_begin_build(os_get_client_area_dims(), os_get_mouse_pos(), font);
  ui_push_font_size(24);
  ui_push_font_size(font.size);


  Scratch scratch = get_scratch(0, 0);

  // Damian: This is the current api for the table, will have to make it better, for now its fine
  UI_Box** array_of_header_boxes = 0;
  U64 number_of_header_boxes     = 0;
  UI_Table_row* array_of_rows    = 0;
  U64 number_of_rows             = 0;
  table_ui_step_1(
    table_conf, ui_px(500), ui_px(500), ArrayCount(process_data_arr), 
    scratch.arena, &array_of_header_boxes, &number_of_header_boxes, &array_of_rows, &number_of_rows
  );

  UI_Col()
  {
    for EachIndex(header_box_index, number_of_header_boxes)
    {
      UI_Parent(array_of_header_boxes[header_box_index])
      {
        if (0) {}
        else if (header_box_index == 0) { ui_text_ellipsed(Str8FromC("Icon")); }
        else if (header_box_index == 1) { ui_text_ellipsed(Str8FromC("Name")); }
        else if (header_box_index == 2) { 
          ui_next_width(ui_grow());
          ui_next_height(ui_grow());
          ui_text_ellipsed(Str8FromC("Usage")); 
        }
      }
    }

    for EachIndex(row_index, number_of_rows)
    {
      UI_Table_row row = array_of_rows[row_index];
      for EachIndex(row_data_entry_index, row.count)
      {
        UI_Box* data_entry_box = row.row_entries[row_data_entry_index];
        UI_Parent(data_entry_box)
        {
          if (0) {}
          else if (row_data_entry_index == 0) { 
            ui_image(process_data_arr[row_index].texture, ui_top_font_size(), ui_top_font_size());
          }
          else if (row_data_entry_index == 1) { 
            ui_text_ellipsed(process_data_arr[row_index].name);
          }
          else if (row_data_entry_index == 2) { 
            ui_text_ellipsed(process_data_arr[row_index].usage_word);
          }
        }

      }
    }

  }

  end_scratch(&scratch);

  ui_end_build();
}

#endif









