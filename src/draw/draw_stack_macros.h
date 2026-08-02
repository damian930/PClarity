#ifndef D_STACK_MACROS_H
#define D_STACK_MACROS_H

#include "draw/draw.h"

#define __D_STACK_DATA_TABLE_EXPANSION(EXPANSION) \
  EXPANSION(__D_Stack_of_blend_kind,    R_Blend_kind, stack_of_blend_kind,    R_Blend_kind__alpha,  d_push_blend_kind,    d_pop_blend_kind,    d_top_blend_kind,    d_blend_kind_stack_has_non_default,    64) \
  EXPANSION(__D_Stack_of_render_target, R_Handle,     stack_of_render_target, r_handle_zero(),      d_push_render_target, d_pop_render_target, d_top_render_target, d_render_target_stack_has_non_default, 64) \
  EXPANSION(__D_Stack_of_scissor_rect,  Rect,         stack_of_scissor_rect,  rect_make(0,0,0,0),   d_push_scissor_rect,  d_pop_scissor_rect,  d_top_scissor_rect,  d_scissor_rect_stack_has_non_default,  64) \
  EXPANSION(__D_Stack_of_fill_mode,     R_Fill_mode,  stack_of_fill_mode,     R_Fill_mode__solid,   d_push_fill_mode,     d_pop_fill_mode,     d_top_fill_mode,     d_fill_mode_stack_has_non_default,     64) 

#define __D_STACK_DEFINE_STACK_STRUCT(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, pop_func_name, get_top_func_name, stack_has_non_default_func_name, stack_arr_capacity) \
struct Stack_type_name { \
  inner_data_type arr[stack_arr_capacity]; \
  U64 count; \
  inner_data_type default_value; \
};

#define __D_STACK_DECLARE_PUSH_FUNC(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, pop_func_name, get_top_func_name, stack_has_non_default_func_name, stack_arr_capacity) \
  void push_func_name(inner_data_type v);

#define __D_STACK_DECLARE_POP_FUNC(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, pop_func_name, get_top_func_name, stack_has_non_default_func_name, stack_arr_capacity) \
  inner_data_type pop_func_name();

#define __D_STACK_DECLARE_TOP_FUNC(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, pop_func_name, get_top_func_name, stack_has_non_default_func_name, stack_arr_capacity) \
  inner_data_type get_top_func_name();

#define __D_STACK_DECLARE_HAS_NON_DEFAULT(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, pop_func_name, get_top_func_name, stack_has_non_default_func_name, stack_arr_capacity) \
  B32 stack_has_non_default_func_name();

#define __D_STACK_DEFINE_PUSH_FUNC(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, pop_func_name, get_top_func_name, stack_has_non_default_func_name, stack_arr_capacity) \
  void push_func_name(inner_data_type v) { \
    Stack_type_name *stack = &d_get_state()->stacks.var_name_inside_state; \
    if (stack->count < ArrayCount(stack->arr)) { \
      stack->arr[stack->count++] = v; \
    } else { \
      BreakPoint("Draw stack overflow. Increase stack capacity."); \
    } \
  }

#define __D_STACK_DEFINE_POP_FUNC(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, pop_func_name, get_top_func_name, stack_has_non_default_func_name, stack_arr_capacity) \
inner_data_type pop_func_name() { \
  Stack_type_name *stack = &d_get_state()->stacks.var_name_inside_state; \
  inner_data_type result = stack->default_value; \
  if (stack->count > 0) { \
    stack->count -= 1; \
    result = stack->arr[stack->count]; \
    stack->arr[stack->count] = {}; \
  } \
  return result; \
}

#define __D_STACK_DEFINE_TOP_FUNC(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, pop_func_name, get_top_func_name, stack_has_non_default_func_name, stack_arr_capacity) \
inner_data_type get_top_func_name() { \
  Stack_type_name *stack = &d_get_state()->stacks.var_name_inside_state; \
  if (stack->count > 0) { \
    return stack->arr[stack->count - 1]; \
  } \
  return stack->default_value; \
}

#define __D_STACK_DEFINE_HAS_NON_DEFAULT(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, pop_func_name, get_top_func_name, stack_has_non_default_func_name, stack_arr_capacity) \
  B32 stack_has_non_default_func_name() { \
    Stack_type_name* stack = &d_get_state()->stacks.var_name_inside_state; \
    return stack->count != 0; \
  }

#endif 