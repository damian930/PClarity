#ifndef PCLARITY_UI_H
#define PCLARITY_UI_H

enum PCL_Command {
  PCL_Command__NONE,
  PCL_Command__go_to_settings,
  PCL_Command__go_to_home,
  PCL_Command__COUNT,
};

enum PCL_Color_name : U32 {
  PCL_Color_name__NONE,
  PCL_Color_name__main_background,
  PCL_Color_name__secondary_background,
  PCL_Color_name__item_selected,
  PCL_Color_name__COUNT,
};

enum PCL_Menu {
  PCL_Menu__home,
  PCL_Menu__settings,
};

struct PCL_State {
  PCL_Command ui_made_command_to_execute;

  PCL_Menu current_menu;
};

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

F32 pcl_ui_slider(F32 width, F32 height, F32 value, RangeF32 range_for_value, Str8 id)
{
  clamp_f32_inplace(&value, range_for_value.min, range_for_value.max);

  // TODO: Round this slider here
  ui_next_width(ui_px(width));
  ui_next_height(ui_px(height));
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
      F32 mouse_to_slider_relative = ui_get_mouse_pos().x - (slider_box_data.rect.origin.x + 2);
      F32 mouse_inside_slider_pos_normalised = mouse_to_slider_relative / (slider_box_data.rect.width - 2);

      new_value = lerp_f32(range_for_value.min, range_for_value.max, mouse_inside_slider_pos_normalised);
    }
  }
  clamp_f32_inplace(&new_value, range_for_value.min, range_for_value.max);

  return new_value;
}

void pcl_init()
{
  pcl_icon_settings         = r_load_texture_from_file(Str8FromC("../data/icons/gear.png"));
  pcl_icon_home             = r_load_texture_from_file(Str8FromC("../data/icons/house.png"));
  pcl_icon_magnifying_glass = r_load_texture_from_file(Str8FromC("../data/icons/magnifying_glass.png"));
}

void pcl_frame_update(PCL_State* PCL)
{
  // todo: Here we do the commands that we have got from the prev frame ui interactions
  switch (PCL->ui_made_command_to_execute)
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
  }
  PCL->ui_made_command_to_execute = PCL_Command__NONE;
}

void pcl_do_ui(FP_Font font, PCL_State* PCL)
{
  Assert(PCL->ui_made_command_to_execute == PCL_Command__NONE); 

  ui_begin_build(os_get_client_area_dims(), os_get_mouse_pos(), font);

  F32 icon_size = ui_top_font_size() * 3; // Damian: For now this is how it is 

  ui_next_width(ui_p_of_p(1));
  ui_next_height(ui_p_of_p(1));
  UI_Row()
  {
    ui_next_width(ui_fit());
    ui_next_height(ui_p_of_p(1));
    ui_next_b_color(pcl_color_from_name(PCL_Color_name__main_background));
    ui_next_padding(pcl_font_size * 0.25f);
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
          PCL->ui_made_command_to_execute = PCL_Command__go_to_home;
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
          PCL->ui_made_command_to_execute = PCL_Command__go_to_settings;
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
      // This here should be a slider for font size

      ui_next_padding(50);
      ui_next_extra_flags(UI_Box_flag__has_padding);
      UI_Col()
      {
        static F32 value = 0.0f; 
        value = pcl_ui_slider(200, 100, value, rangeF32(0, 100), Str8FromC("Test slider id"));
        ui_spacer(ui_px(15));
        ui_label_f("%.0f \n", value);
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

  }

  ui_end_build();
}

#endif









