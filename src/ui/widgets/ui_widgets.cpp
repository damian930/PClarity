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
// - Simple wrapper
//
void ui_begin_wrapper()
{
  UI_Box* box = ui_box_make({}, UI_Box_flag__NONE);
  ui_push_parent(box);
}

void ui_end_wrapper()
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
  V2F32 str_dims = fp_measure_text(outer_str, font, ui_top_font_size());
  
  ui_next_width(ui_px(str_dims.x));
  ui_next_height(ui_px(str_dims.y));
  UI_Box* box = ui_box_make({}, 0);

  // Damian, TODO: Now that i think about this, this might not really be used that much
  //               and if not then remove the extension from the UI_Box in the ui layer
  //               and just allocated the string as custom data for label custom draw function
  ui_extend_box_with_text(box, outer_str);
  ui_extend_box_with_custom_draw_function(box, __ui_label_draw_func, Null);
}

void ui_label_f(const char* fmt, ...)
{
  ScratchLoop(scratch, 0, 0)
  {
    va_list argptr;
    va_start(argptr, fmt);
    Str8 str = str8_valist(scratch.arena, fmt, argptr);
    ui_label(str);
    va_end(argptr);
  }
}

void ui_text(Str8 str) 
{ 
  ui_label(str); 
}

void ui_text_f(const char* fmt, ...) 
{ 
  ScratchLoop(scratch, 0, 0)
  {
    va_list argptr;
    va_start(argptr, fmt);
    Str8 str = str8_valist(scratch.arena, fmt, argptr);
    ui_text(str);
    va_end(argptr);
  }
}

UI_CUSTOM_DRAW_BOX_DEF(__ui_label_draw_func)
{
  UI_Box* box      = provided_data.box;
  Str8 text        = box->text_extension.text;
  FP_Font font     = box->text_extension.font;
  F32 font_size    = box->text_extension.font_size;
  V4F32 font_color = box->text_extension.font_color;
  d_draw_text(text, font, font_size, provided_data.final_box_rect.origin, font_color);
}

///////////////////////////////////////////////////////////
// - Ellipsed labels 
//
UI_CUSTOM_DRAW_BOX_DEF(__ui_label_ellipsed_draw_func)
{
  Scratch scratch = get_scratch(0, 0);

  UI_Box* box = provided_data.box;  
  Rect rect   = provided_data.final_box_rect;

  Str8 final_str_to_draw = box->text_extension.text;

  V2F32 text_dims = fp_measure_text(box->text_extension.text, box->text_extension.font, box->text_extension.font_size);
  if (text_dims.x > rect.width)
  {
    Str8 ellipsis = Str8FromC("...");
    V2F32 ellissis_dims = fp_measure_text(ellipsis, box->text_extension.font, box->text_extension.font_size);

    F32 text_width_after_ellissing = rect.width - ellissis_dims.x;
    if (text_width_after_ellissing < 0)
    {
      final_str_to_draw = {};
    }
    else 
    {
      RangeU64 range_that_fits = fp_get_text_range_that_fits(box->text_extension.text, text_width_after_ellissing, box->text_extension.font, box->text_extension.font_size);
      Str8 visible_part = str8_substring_range(box->text_extension.text, range_that_fits);

      Str8_list list = {};
      str8_list_append_view(scratch.arena, &list, visible_part);
      str8_list_append_view(scratch.arena, &list, ellipsis);
      final_str_to_draw = str8_from_list(scratch.arena, &list);
    }
  }

  d_draw_text(final_str_to_draw, box->text_extension.font, box->text_extension.font_size, rect.origin, white());

  end_scratch(&scratch);
}

void ui_label_ellipsed(Str8 str)
{
  // Damian: Ellipsed label is not sized to fit the text, size commes from the outside
  UI_Box* box = ui_box_make({}, UI_Box_flag__NONE);
  ui_extend_box_with_text(box, str);
  ui_extend_box_with_custom_draw_function(box, __ui_label_ellipsed_draw_func, Null);
}

void ui_label_ellipsed_f(const char* fmt, ...)
{
  ScratchLoop(scratch, 0, 0)
  {
    va_list argptr;
    va_start(argptr, fmt);
    Str8 str = str8_valist(scratch.arena, fmt, argptr);
    ui_label_ellipsed(str);
    va_end(argptr);
  }
}

void ui_text_ellipsed(Str8 str)
{
  ui_label_ellipsed(str);
}

void ui_text_ellipsed_f(const char* fmt, ...)
{
  ScratchLoop(scratch, 0, 0)
  {
    va_list argptr;
    va_start(argptr, fmt);
    Str8 str = str8_valist(scratch.arena, fmt, argptr);
    ui_text_ellipsed(str);
    va_end(argptr);
  }
}

///////////////////////////////////////////////////////////
// - Images
//
UI_CUSTOM_DRAW_BOX_DEF(__ui_image_draw_func)
{
  R_Handle texture = *((R_Handle*)provided_data.box->custom_draw_extension.data_for_draw_func);
  Rect texture_rect = rect_make_v(v2f32(0.0f, 0.0f), r_get_handle_dims(texture));
  d_draw_texture_pro(texture, provided_data.final_box_rect, texture_rect, white());
}
void ui_image(R_Handle texture, F32 width_px, F32 height_px)
{
  V2F32 dims = r_get_handle_dims(texture);
  ui_next_width(ui_px(width_px));
  ui_next_height(ui_px(height_px));
  UI_Box* box = ui_box_make({}, 0);

  R_Handle* handle = ArenaPush(ui_get_build_arena(), R_Handle);
  *handle = texture;
  
  ui_extend_box_with_custom_draw_function(box, __ui_image_draw_func, (void*)handle);
}
#undef UI_CUSTOM_DATA_FOR_IMAGE



#endif