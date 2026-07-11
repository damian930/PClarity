#ifndef PCLARITY_UI_H
#define PCLARITY_UI_H

enum PCL_Color_name : U32 {
  PCL_Color_name__NONE,
  PCL_Color_name__main_background,
  PCL_Color_name__secondary_background,
  PCL_Color_name__COUNT,
};

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

void pcl_do_ui(FP_Font font)
{
  ui_begin_build(os_get_window_dims(), os_get_mouse_pos(), font);

  ui_next_width(ui_px(100));
  ui_next_height(ui_p_of_p(1));
  ui_next_b_color(pcl_color_from_name(PCL_Color_name__main_background));
  UI_Box* box = ui_box_make_f("Left box id", UI_Box_flag__has_background);
  UI_Parent(box)
  {
    ui_next_font_size(64);
    ui_label(Str8FromC("_ICON_"));


  }

  ui_next_width(ui_px(1));
  ui_next_height(ui_p_of_p(1));
  ui_next_b_color(magenta());
  UI_Box* sep = ui_box_make_f("Sep id", UI_Box_flag__has_background);
  
  

  ui_end_build();

}

#endif









