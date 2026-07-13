#ifndef __UI_WIDGETS_CPP
#define __UI_WIDGETS_CPP

#include "ui/ui_core.h"
#include "ui/ui_core.cpp"
#include "ui/widgets/ui_widgets.h"

///////////////////////////////////////////////////////////
// - Simple widgets for quick
//
UI_Actions ui_button(Str8 id)
{
  UI_Box* button_box = ui_box_make(id, 
    UI_Box_flag__has_background|
    UI_Box_flag__has_rounded_corners|
    UI_Box_flag__has_borders
  );
  UI_Actions acts = ui_actions_from_box(button_box);
  return acts;
}

void ui_spacer(UI_Size size)
{
  UI_Box* parent = ui_top_parent();
  if (0) {}
  else if (parent->clay_element_config.layout.layoutDirection == CLAY_LEFT_TO_RIGHT) { ui_next_width(size); ui_next_height(ui_px(0.0f)); }
  else if (parent->clay_element_config.layout.layoutDirection == CLAY_TOP_TO_BOTTOM) { ui_next_height(size); ui_next_width(ui_px(0.0f)); }
  ui_box_make(Str8{}, 0);
}

///////////////////////////////////////////////////////////
// - Layout stacks
//
void ui_begin_layout_stack(Axis2 axis)
{
  ui_next_layout(axis);
  UI_Box* box = ui_box_make({}, 0);
  ui_push_parent(box);
}
void ui_end_layout_stack()
{
  ui_pop_parent();
}

///////////////////////////////////////////////////////////
// - Labels
//
UI_CUSTOM_DRAW_BOX_DEF(__ui_label_draw_func);

void ui_label(Str8 outer_str)
{
  FP_Font font  = ui_top_font();
  F32 font_size = ui_top_font_size();

  V2F32 str_dims = fp_measure_text(outer_str, font);
  
  ui_next_width(ui_px(str_dims.x));
  ui_next_height(ui_px(str_dims.y));
  UI_Box* box = ui_box_make({}, 0);

  struct draw_data {
    Str8 str;
    FP_Font font;
    F32 font_size;
  };

  draw_data* data = ArenaPush(ui_get_state()->build_arena, draw_data);
  data->str       = outer_str;
  data->font      = font;
  data->font_size = font_size;
  data->str.data  = ArenaPushArr(ui_get_state()->build_arena, U8, outer_str.count);
  data->str.count = outer_str.count;
  memcpy(data->str.data, outer_str.data, data->str.count);

  ui_extend_box_with_custom_draw_function(box, __ui_label_draw_func, data);
}

void ui_label_f(const char* fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  Scratch scratch = get_scratch(0, 0);
  U64 buffer_count = 128;
  U8* buffer = ArenaPushArr(scratch.arena, U8, buffer_count);
  int err = vsnprintf((char*)buffer, buffer_count, fmt, argptr);
  if (err < 0) { Assert(0); }
  else if (err >= buffer_count) { Assert(0); }
  else if (err < buffer_count) { /* All good */ }
  va_end(argptr);
  Str8 str = str8_manual_view(buffer, (U64)err);
  ui_label(str);
  end_scratch(&scratch);
}

UI_CUSTOM_DRAW_BOX_DEF(__ui_label_draw_func)
{
  struct draw_data {
    Str8 str;
    FP_Font font;
    F32 font_size;
  };

  draw_data* data = (draw_data*)custom_data;
  d_draw_text(data->str, data->font, data->font_size, provided_data.final_box_rect.origin, white());
}

///////////////////////////////////////////////////////////
// - Images
//
#define UI_CUSTOM_DATA_FOR_IMAGE(var_name) \
struct var_name { \
  R_Handle texture; \
};
UI_CUSTOM_DRAW_BOX_DEF(__ui_image_draw_func)
{
  UI_CUSTOM_DATA_FOR_IMAGE(Custom_data);
  Custom_data* data = (Custom_data*)custom_data;

  Rect texture_rect = rect_make_v(v2f32(0.0f, 0.0f), r_get_handle_dims(data->texture));
  d_draw_texture_pro(data->texture, provided_data.final_box_rect, texture_rect, white());
}
void ui_image(R_Handle texture, F32 width_px, F32 height_px)
{
  V2F32 dims = r_get_handle_dims(texture);

  ui_next_width(ui_px(width_px));
  ui_next_height(ui_px(height_px));
  UI_Box* box = ui_box_make({}, 0);

  UI_CUSTOM_DATA_FOR_IMAGE(Custom_data);
  Custom_data* custom_data = ArenaPush(ui_get_build_arena(), Custom_data);
  custom_data->texture = texture;

  ui_extend_box_with_custom_draw_function(box, __ui_image_draw_func, (void*)custom_data);
}
#undef UI_CUSTOM_DATA_FOR_IMAGE



#endif