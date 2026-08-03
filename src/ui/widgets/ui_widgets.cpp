#ifndef __UI_WIDGETS_CPP
#define __UI_WIDGETS_CPP

#include "ui/ui_core.h"
#include "ui/ui_core.cpp"
#include "ui/widgets/ui_widgets.h"

///////////////////////////////////////////////////////////
// - Layout stacks
//
void ui_begin_layout_stack_flagged(Axis2 axis, UI_Box_flags flags)
{
  ui_next_layout(axis);
  UI_Box* box = ui_box_make(flags, {});
  ui_push_parent(box);
}

void ui_begin_layout_stack(Axis2 axis)
{
  ui_begin_layout_stack_flagged(axis, UI_Box_flag__NONE);
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
  UI_Box* box = ui_box_make(UI_Box_flag__NONE, {});
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
  F32 font_size = ui_top_font_size();
  V2F32 str_dims = fp_measure_text(outer_str, font, font_size);
  
  ui_next_width(ui_px(str_dims.x));
  ui_next_height(ui_px(str_dims.y));
  UI_Box* box = ui_box_make(0, {});

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

void __ui_label_draw_func(UI_Box* box)
{
  Str8 text        = box->per_build_config.text_extension.text;
  FP_Font font     = box->per_build_config.text_extension.font;
  F32 font_size    = box->per_build_config.text_extension.font_size;
  V4F32 font_color = box->per_build_config.text_extension.font_color;
  Rect rect        = box->rect;
  d_draw_text(text, font, font_size, rect.origin, font_color);
}

///////////////////////////////////////////////////////////
// - Ellipsed labels 
//
void __ui_label_ellipsed_draw_func(UI_Box* box )
{
  Scratch scratch = get_scratch(0, 0);

  Rect rect        = box->rect;
  Str8 text        = box->per_build_config.text_extension.text;
  FP_Font font     = box->per_build_config.text_extension.font;
  F32 font_size    = box->per_build_config.text_extension.font_size;
  V4F32 font_color = box->per_build_config.text_extension.font_color;

  Str8 final_str_to_draw = text;

  V2F32 text_dims = fp_measure_text(text, font, font_size);
  if (text_dims.x > rect.width)
  {
    Str8 ellipsis = Str8FromC("...");
    V2F32 ellissis_dims = fp_measure_text(ellipsis, font, font_size);

    F32 text_width_after_ellissing = rect.width - ellissis_dims.x;
    if (text_width_after_ellissing < 0)
    {
      final_str_to_draw = {};
    }
    else 
    {
      RangeU64 range_that_fits = fp_get_text_range_that_fits(text, text_width_after_ellissing, font, font_size);
      Str8 visible_part = str8_substring_range(text, range_that_fits);

      Str8_list list = {};
      str8_list_append_view(scratch.arena, &list, visible_part);
      str8_list_append_view(scratch.arena, &list, ellipsis);
      final_str_to_draw = str8_from_list(scratch.arena, &list);
    }
  }

  d_draw_text(final_str_to_draw, font, font_size, rect.origin, white());

  end_scratch(&scratch);
}

void ui_label_ellipsed(Str8 str)
{
  // Damian: Ellipsed label is not sized to fit the text, size commes from the outside
  V2F32 text_dims = fp_measure_text(str, ui_top_font(), ui_top_font_size());
  
  ui_next_width(ui_grow());
  ui_next_height(ui_px(text_dims.y));
  UI_Box* box = ui_box_make(UI_Box_flag__NONE, {});
  
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
// - Butoon
//
UI_Actions ui_button(Str8 id_and_text)
{
  F32 font_size    = ui_top_font_size();
  FP_Font font     = ui_top_font();
  V4F32 font_color = ui_top_font_color();

  ui_next_alignment_x(UI_Alignment_x__center);
  ui_next_alignment_y(UI_Alignment_y__center);
  UI_Box* button_box = ui_box_make(
    UI_Box_flag__has_padding|
    UI_Box_flag__has_borders|
    UI_Box_flag__has_background|
    UI_Box_flag__has_rounded_corners|
    UI_Box_flag__clickable,
    id_and_text
  );
  UI_Parent(button_box)
  {
    // TODO: Use ellipsed text here
    // Str8 text = ui_get_text_part_from_str(id_and_text);
    Str8 text = id_and_text;
  
    ui_next_font_size(font_size);
    ui_next_font(font);
    ui_next_font_color(font_color);
    ui_text(text);
  }
  UI_Actions actions = ui_actions_from_box(button_box);
  return actions;
}

UI_Actions ui_button_f(const char* fmt, ...)
{
  UI_Actions actions = {};
  ScratchLoop(scratch, 0, 0)
  {
    va_list argptr;
    va_start(argptr, fmt);
    Str8 str = str8_valist(scratch.arena, fmt, argptr);
    actions = ui_button(str);
    va_end(argptr);
  }
  return actions;
}

///////////////////////////////////////////////////////////
// - Spacer
//
void ui_spacer(UI_Size size)
{
  UI_Box* parent = ui_top_parent();
  if (0) {}
  else if (parent->per_build_config.layout_direction == Axis2__x) { ui_next_width(size); ui_next_height(ui_px(0.0f)); }
  else if (parent->per_build_config.layout_direction == Axis2__y) { ui_next_height(size); ui_next_width(ui_px(0.0f)); }
  ui_box_make(0, {});
}

///////////////////////////////////////////////////////////
// - Images
//
void __ui_image_draw_func(UI_Box* box)
{
  R_Handle texture = *((R_Handle*)box->per_build_config.custom_draw_extension.data_for_draw_func);
  Rect texture_rect = rect_make_v(v2f32(0.0f, 0.0f), r_get_handle_dims(texture));
  d_draw_texture_pro(texture, box->rect, texture_rect, white());
}
void ui_image(R_Handle texture, F32 width_px, F32 height_px)
{
  V2F32 dims = r_get_handle_dims(texture);
  ui_next_width(ui_px(width_px));
  ui_next_height(ui_px(height_px));
  UI_Box* box = ui_box_make(0, {});

  R_Handle* handle = ArenaPush(ui_get_build_arena(), R_Handle);
  *handle = texture;
  
  ui_extend_box_with_custom_draw_function(box, __ui_image_draw_func, (void*)handle);
}
#undef UI_CUSTOM_DATA_FOR_IMAGE

///////////////////////////////////////////////////////////
// - Color pickers (Saturation + Value)
//
struct __UI_Color_picker_sv_data {
  V4F32 colors[UV__COUNT];
};

void ui_color_picker_sv(Str8 id, UI_Size size_x, UI_Size size_y, V4F32 hsva, F32* out_opt_new_sat, F32* out_opt_new_val)
{
  // DD:
  // this picker is for sv, meaning for saturation and value, these are hsv values, not rgb
  // the value goes bottom-up in the color picker
  // the saturation goes left-right in the color picker
  // the bottom left and right are black
  // the top left is white
  // the top right is the purest version of the color. This is represented by hue, but in the rgb world this
  // would have to be rgba_from_hsva(hue, 1.0f, 1.0f), so both value and saturation are 1.0s, this gives the most 
  // saturated and brigth color for a shade of color, which is specified by the hue, which is a 0->360* or
  // 0->1.0f value of the hsv color pallet. 

  // DD: Plan on the order of code:
  // 1) Do the ui based on the current state of the data
  //    - Draw the picker based on the provider prev color
  // 2) Based on the inputs, update the data and just give the new data to the user, keep the old one
  //    - Based on mouse pos + color picker dims, get the new color, return to the user
  
  // Setting up the color picke box
  ui_next_width(size_x);
  ui_next_height(size_y);
  UI_Box* color_picker_box = ui_box_make(
    UI_Box_flag__has_padded_border|
    UI_Box_flag__left_clickable, 
    id
  );

  UI_Box_data color_picker_data = ui_box_data_from_box(color_picker_box);
  V2F32 mouse          = ui_get_mouse_pos();
  F32 circle_diameter  = 10.0f;

  UI_Parent(color_picker_box)
  {
    ui_next_width(ui_grow());
    ui_next_height(ui_grow());
    UI_Box* _color_picker_box_for_custom_draw = ui_box_make(UI_Box_flag__dont_draw_overflow, {});
  
    V4F32 pure_hsv = v4f32(hsva.hue, 1.0f, 1.0f, 1.0f);
    
    __UI_Color_picker_sv_data* draw_data = ArenaPush(ui_get_build_arena(), __UI_Color_picker_sv_data);
    draw_data->colors[UV__top_left]     = white(); 
    draw_data->colors[UV__top_right]    = rgba_from_hsva(pure_hsv);
    draw_data->colors[UV__bottom_left]  = black();
    draw_data->colors[UV__bottom_right] = black();
    ui_extend_box_with_custom_draw_function(_color_picker_box_for_custom_draw, __ui_color_picker_sv_square_draw_func, draw_data);

    F32 circle_x_offset = 0.0f;
    F32 circle_y_offset = 0.0f;
    if (color_picker_data.is_found)
    {
      // Reverse lerp
      F32 x_t = hsva.saturation;
      F32 y_t = hsva.value ;
      clamp_f32_inplace(&x_t, 0.0f, 1.0f);
      clamp_f32_inplace(&y_t, 0.0f, 1.0f);

      // TODO: Might have to use inner rect here, look into this
      circle_x_offset = (color_picker_data.inner_rect.width * x_t) - (circle_diameter / 2.0f);
      circle_y_offset = (color_picker_data.inner_rect.height * (1.0f - y_t)) - (circle_diameter / 2.0f);
    }

    UI_Parent(_color_picker_box_for_custom_draw)
    {
      ui_next_floating_fixed_pos(v2f32(circle_x_offset, circle_y_offset));
      ui_next_floating_fixed_dims(v2f32(circle_diameter, circle_diameter));
      ui_next_border(2, white());
      ui_next_corner_r(f32_max_decimal);
      ui_next_inner_softness(2);
      ui_next_outer_softness(2);
      UI_Box* picking_circle_floating_parent = ui_box_make(UI_Box_flag__floating|UI_Box_flag__has_borders|UI_Box_flag__has_rounded_corners, {});
    }

  }

  // Updating the colors 
  F32 new_sat = hsva.saturation;
  F32 new_val = hsva.value;
  UI_Actions actions = ui_actions_from_box(color_picker_box);
  if (actions.is_down) 
  {
    if (color_picker_data.is_found)
    {
      // TODO: Mighte have to use inner rect here, look into this
      F32 picker_relative_x = (mouse.x - color_picker_data.inner_rect.x) / (color_picker_data.inner_rect.width);
      F32 picker_relative_y = 1.0f - ((mouse.y - color_picker_data.inner_rect.y) / (color_picker_data.inner_rect.height)); // Flipping the Y since color picker is bottom_left->up and the screen is top_left->down
      clamp_f32_inplace(&picker_relative_x, 0.0f, 1.0f);
      clamp_f32_inplace(&picker_relative_y, 0.0f, 1.0f);

      // Getting the color for the realative mouse pos
      new_sat = picker_relative_x;
      new_val = picker_relative_y;
    }
  }
  if (out_opt_new_sat) { *out_opt_new_sat = new_sat; }
  if (out_opt_new_val) { *out_opt_new_val = new_val; }
}

void __ui_color_picker_sv_square_draw_func(UI_Box* box)
{
  __UI_Color_picker_sv_data* data = (__UI_Color_picker_sv_data*)box->per_build_config.custom_draw_extension.data_for_draw_func;
  Rect rect = box->rect;
  V4F32 color_at_corners[UV__COUNT] = { data->colors[UV__x0y0], data->colors[UV__x1y0], data->colors[UV__x0y1], data->colors[UV__x1y1] };
  d_draw_rect_pro(rect, color_at_corners, v4f32_all(0.0f), 0.0f, transparent(), 0.0f, 0.0f);
}

#endif