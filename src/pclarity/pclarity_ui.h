#ifndef PCLARITY_UI_H
#define PCLARITY_UI_H

enum PCL_Color_name : U32 {
  PCL_Color_name__NONE,
  PCL_Color_name__main_background,
  PCL_Color_name__secondary_background,
  PCL_Color_name__COUNT,
};

global F32 pcl_font_size      = 24.0f;
global R_Handle icon_settings = r_zero_handle();
global R_Handle icon_home     = r_zero_handle();

V4F32 __pcl_g_color_values_for_names[PCL_Color_name__COUNT] = {
  rgba_from_hex(0x00000000) , // NONE
  rgba_from_hex(0x191432FF),
  rgba_from_hex(0x362753FF),
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

void pcl_init()
{
  icon_settings = r_load_texture_from_file(Str8FromC("../data/icons/gear.png"));
  icon_home     = r_load_texture_from_file(Str8FromC("../data/icons/house.png"));
}

void pcl_do_ui(FP_Font font)
{
  ui_begin_build(os_get_client_area_dims(), os_get_mouse_pos(), font);

  ui_next_width(ui_fit());
  ui_next_height(ui_p_of_p(1));
  ui_next_b_color(pcl_color_from_name(PCL_Color_name__main_background));
  ui_next_padding(pcl_font_size * 0.25f);
  ui_next_layout_y();
  ui_next_alignment_x(UI_Alignment_x__center);
  UI_Box* box = ui_box_make_f("Left box id", UI_Box_flag__has_background|UI_Box_flag__has_padding);
  UI_Parent(box)
  {
    F32 icon_size = ui_top_font_size() * 3;

    // TODO: On hover have a different color
    ui_next_width(ui_fit()); ui_next_height(ui_fit()); ui_next_padding(ui_top_font_size());
    ui_next_hover_cursor(OS_Cursor__hand);
    UI_Box* test_box = ui_box_make(Str8FromC("Test id for first button"), UI_Box_flag__has_background);
    UI_Parent(test_box)
    {
      ui_image(icon_home, icon_size, icon_size);
    }
    UI_Actions box_acts = ui_actions_from_box(test_box);
    if (box_acts.is_hovered) {
      ui_set_box_b_color(test_box, orange());
    }

    ui_spacer(ui_grow());

    UI_Width(ui_fit()) UI_Height(ui_fit()) UI_Padding(ui_top_font_size()) 
    {
      ui_image(icon_settings, icon_size, icon_size);
    }

  }

  ui_next_width(ui_px(1));
  ui_next_height(ui_p_of_p(1));
  ui_next_b_color(magenta());
  UI_Box* sep = ui_box_make_f("Sep id", UI_Box_flag__has_background);
  
  

  ui_end_build();

}

#endif









