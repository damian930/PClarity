#ifndef UI_STACK_MACROS_H
#define UI_STACK_MACROS_H

#include "ui/ui_core.h"

// Complete style stacks data 
#define __UI_STACK_DATA_TABLE_EXPANSION(EXPANSION) \
  EXPANSION(UI_Size_x_stack,         UI_Size, stack_size_x,         ui_fit(), ui_push_size_x,         ui_next_size_x,         ui_pop_size_x,         ui_auto_pop_size_x,         ui_top_size_x) \
  EXPANSION(UI_Size_y_stack,         UI_Size, stack_size_y,         ui_fit(), ui_push_size_y,         ui_next_size_y,         ui_pop_size_y,         ui_auto_pop_size_y,         ui_top_size_y) \
  \
  EXPANSION(UI_Child_gap_stack,      F32,     stack_child_gap,      0.0f,     ui_push_child_gap,      ui_next_child_gap,      ui_pop_child_gap,      ui_auto_pop_child_gap,      ui_top_child_gap) \
  \
  EXPANSION(UI_Padding_left_stack,   F32,     stack_padding_left,   0.0f,     ui_push_padding_left,   ui_next_padding_left,   ui_pop_paddiing_left,   ui_auto_pop_padding_left,   ui_top_padding_left) \
  EXPANSION(UI_Padding_top_stack,    F32,     stack_padding_top,    0.0f,     ui_push_padding_top,    ui_next_padding_top,    ui_pop_paddiing_top,    ui_auto_pop_padding_top,    ui_top_padding_top) \
  EXPANSION(UI_Padding_right_stack,  F32,     stack_padding_right,  0.0f,     ui_push_padding_right,  ui_next_padding_right,  ui_pop_paddiing_right,  ui_auto_pop_padding_right,  ui_top_padding_right) \
  EXPANSION(UI_Padding_bottom_stack, F32,     stack_padding_bottom, 0.0f,     ui_push_padding_bottom, ui_next_padding_bottom, ui_pop_paddiing_bottom, ui_auto_pop_padding_bottom, ui_top_padding_bottom) \
  \
  EXPANSION(UI_Layout_axis_stack, Axis2, stack_layout_axis, Axis2__y,     ui_push_layout, ui_next_layout, ui_pop_layout, ui_auto_pop_layout, ui_top_layout) \
  \
  EXPANSION(UI_Background_color_stack, V4F32, stack_background_color, v4f32_all(0.0f), ui_push_background_color, ui_next_background_color, ui_pop_background_color, ui_auto_pop_background_color, ui_top_background_color) \
  \
  EXPANSION(UI_Corner_radius_top_left_stack,     F32, stack_corner_radius_top_left,     0.0f, ui_push_corner_radius_top_left,     ui_next_corner_radius_top_left,     ui_pop_corner_radius_top_left,     ui_auto_pop_corner_radius_top_left,     ui_top_corner_radius_top_left) \
  EXPANSION(UI_Corner_radius_top_right_stack,    F32, stack_corner_radius_top_right,    0.0f, ui_push_corner_radius_top_right,    ui_next_corner_radius_top_right,    ui_pop_corner_radius_top_right,    ui_auto_pop_corner_radius_top_right,    ui_top_corner_radius_top_right) \
  EXPANSION(UI_Corner_radius_bottom_right_stack, F32, stack_corner_radius_bottom_right, 0.0f, ui_push_corner_radius_bottom_right, ui_next_corner_radius_bottom_right, ui_pop_corner_radius_bottom_right, ui_auto_pop_corner_radius_bottom_right, ui_top_corner_radius_bottom_right) \
  EXPANSION(UI_Corner_radius_bottom_left_stack,  F32, stack_corner_radius_bottom_left,  0.0f, ui_push_corner_radius_bottom_left,  ui_next_corner_radius_bottom_left,  ui_pop_corner_radius_bottom_left,  ui_auto_pop_corner_radius_bottom_left,  ui_top_corner_radius_bottom_left) \
  \
  EXPANSION(UI_Border_color_stack, V4F32, stack_border_color, v4f32_all(0.0f), ui_push_border_color, ui_next_border_color, ui_pop_border_color, ui_auto_pop_border_color, ui_top_border_color) \
  \
  EXPANSION(UI_Border_left_stack,   F32, stack_border_left,   0.0f, ui_push_border_left,   ui_next_border_left,   ui_pop_border_left,   ui_auto_pop_border_left,   ui_top_border_left) \
  EXPANSION(UI_Border_right_stack,  F32, stack_border_right,  0.0f, ui_push_border_right,  ui_next_border_right,  ui_pop_border_right,  ui_auto_pop_border_right,  ui_top_border_right) \
  EXPANSION(UI_Border_top_stack,    F32, stack_border_top,    0.0f, ui_push_border_top,    ui_next_border_top,    ui_pop_border_top,    ui_auto_pop_border_top,    ui_top_border_top) \
  EXPANSION(UI_Border_bottom_stack, F32, stack_border_bottom, 0.0f, ui_push_border_bottom, ui_next_border_bottom, ui_pop_border_bottom, ui_auto_pop_border_bottom, ui_top_border_bottom) 

#define __UI_STACK_DEFINE_STACK_STRUCTS(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, set_next_func_name, pop_func_name, auto_pop_func_name, get_top_func_name) \
  struct Stack_type_name { \
    inner_data_type arr[64]; \
    U64 count; \
    \
    inner_data_type single_use_value; \
    B32 is_single_use_value_set; \
    \
    inner_data_type default_value; \
  };

#define __UI_STACK_DECLARE_PUSH_FUNC(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, set_next_func_name, pop_func_name, auto_pop_func_name, get_top_func_name) \
  void push_func_name(inner_data_type v);

#define __UI_STACK_DECLARE_POP_FUNC(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, set_next_func_name, pop_func_name, auto_pop_func_name, get_top_func_name) \
  inner_data_type pop_func_name();

#define __UI_STACK_DECLARE_AUTO_POP_FUNC(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, set_next_func_name, pop_func_name, auto_pop_func_name, get_top_func_name) \
  inner_data_type auto_pop_func_name();

#define __UI_STACK_DECLARE_GET_FUNC(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, set_next_func_name, pop_func_name, auto_pop_func_name, get_top_func_name) \
  inner_data_type get_top_func_name();

#define __UI_STACK_DEFINE_PUSH_FUNC(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, set_next_func_name, pop_func_name, auto_pop_func_name, get_top_func_name) \
  void push_func_name(inner_data_type v) { \
    if (ui_get_state()->var_name_inside_state.count < ArrayCount(ui_get_state()->var_name_inside_state.arr)) { \
      ui_get_state()->var_name_inside_state.arr[ui_get_state()->var_name_inside_state.count++] = v; \
    } else { \
      BreakPoint("Make stacks bigger, the prev size is too small it seems"); \
    } \
  }

#define __UI_STACK_DEFINE_SET_NEXT_FUNC(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, set_next_func_name, pop_func_name, auto_pop_func_name, get_top_func_name) \
  void set_next_func_name(inner_data_type v) { \
    ui_get_state()->var_name_inside_state.single_use_value        = v; \
    ui_get_state()->var_name_inside_state.is_single_use_value_set = true; \
  }

#define __UI_STACK_DEFINE_POP_FUNC(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, set_next_func_name, pop_func_name, auto_pop_func_name, get_top_func_name) \
  inner_data_type pop_func_name() { \
    Stack_type_name* stack = &ui_get_state()->var_name_inside_state; \
    inner_data_type return_prev_value = stack->default_value; \
    if (stack->is_single_use_value_set) \
    { \
      inner_data_type* prev_value_p = &stack->single_use_value; \
      return_prev_value             = *prev_value_p; \
      *prev_value_p                 = {}; \
      stack->is_single_use_value_set = false; \
    } \
    else if (stack->count > 0) \
    { \
      stack->count -= 1; \
      inner_data_type* prev_value_p = &stack->arr[stack->count]; \
      return_prev_value             = *prev_value_p; \
      *prev_value_p                 = {}; \
    } \
    return return_prev_value; \
  }

#define __UI_STACK_DEFINE_AUTO_POP_FUNC(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, set_next_func_name, pop_func_name, auto_pop_func_name, get_top_func_name) \
  inner_data_type auto_pop_func_name() { \
    Stack_type_name* stack = &ui_get_state()->var_name_inside_state; \
    inner_data_type return_prev_value = stack->default_value; \
    if (stack->is_single_use_value_set) { \
      return_prev_value              = stack->single_use_value; \
      stack->single_use_value        = {}; \
      stack->is_single_use_value_set = false; \
    } \
    return return_prev_value; \
  }

#define __UI_STACK_DEFINE_TOP_FUNC(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, set_next_func_name, pop_func_name, auto_pop_func_name, get_top_func_name) \
  inner_data_type get_top_func_name() { \
    Stack_type_name* stack = &ui_get_state()->var_name_inside_state; \
    inner_data_type return_top_value = stack->default_value; \
    if (stack->is_single_use_value_set) { \
      return_top_value = stack->single_use_value; \
    } \
    else if (stack->count > 0) { \
      return_top_value = stack->arr[stack->count - 1]; \
    } \
    return return_top_value; \
  }








#endif UI_STACK_MACROS_H