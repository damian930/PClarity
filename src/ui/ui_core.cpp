#ifndef __UI_CPP
#define __UI_CPP

#include "core/core_include.h"
#include "core/core_include.cpp"

#include "font_provider/font_provider.h"
#include "font_provider/font_provider.cpp"

#include "ui/ui_core.h"

// note: Clay got some warning, so we just gonna disable them
#pragma warning(disable: 4244)
#pragma warning(disable: 4305)
#ifndef CLAY_IMPLEMENTATION
#define CLAY_IMPLEMENTATION
#include "__third_party/clay/clay.h"
#endif
#pragma warning(default: 4244)
#pragma warning(default: 4305)

UI_State* __ui_g_state = 0;

// TODO: Move this to a better place
void __ui_error_handler_for_clay(Clay_ErrorData errorText)
{
  BreakPoint();
}

///////////////////////////////////////////////////////////
// - State
//
UI_State* ui_get_state()
{
  return __ui_g_state;
}

void ui_set_state(UI_State* state)
{
  __ui_g_state = state;
}

void ui_init()
{
  Arena* state_arena = arena_alloc(Kilobytes(16));
  __ui_g_state = ArenaPush(state_arena, UI_State);
  __ui_g_state->state_arena = state_arena;

  U64 mem_size_for_clay    = Clay_MinMemorySize();
  Arena* arena_for_clay    = arena_alloc(mem_size_for_clay);
  U8* bytes_for_clay_arena = ArenaPushArr(arena_for_clay, U8, mem_size_for_clay);
  Clay_Arena arena         = Clay_CreateArenaWithCapacityAndMemory(mem_size_for_clay, bytes_for_clay_arena);
  Clay_Initialize(arena, Clay_Dimensions{ 100, 100 }, Clay_ErrorHandler{ __ui_error_handler_for_clay, 0 });
  __ui_g_state->arena_for_clay = arena_for_clay;

  // TODO: This is test code
  // TODO: Release this in the ui_release func
  __ui_g_state->build_arena = arena_alloc(Megabytes(4));
}

void ui_release()
{
  arena_release(&__ui_g_state->arena_for_clay);
  arena_release(&__ui_g_state->state_arena);
  __ui_g_state = 0;
}

///////////////////////////////////////////////////////////
// - UI building
//
void ui_begin_build(V2F32 window_dims, V2F32 mouse_pos)
{   
  { // Making sure that null box has not been modified last frame by someone 
    // TODO: This assert breaks, fix this
    B32 comp = {};
    UI_Box valid_null_box = __UI_NULL_BOX_VALUE;
    MemCompareSafe(__ui_g_null_box, valid_null_box, &comp);
    #if 0 // Commented it out for now
    Assert(comp);
    #endif
  }
  
  UI_State* state = ui_get_state();

  state->build_generation += 1;

  // Resetting all the stacks
  #define UI_RESET_STACKS(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, set_next_func_name, pop_func_name, auto_pop_func_name, get_top_func_name, stack_arr_capacity, defer_push_pop_macro_name) \
    state->stacks.var_name_inside_state = {};
  __UI_STACK_DATA_TABLE_EXPANSION(UI_RESET_STACKS)
  #undef UI_RESET_STACKS

  arena_clear(state->build_arena);
  state->render_commands_as_result_of_ui_build = {};

  ui_next_width(ui_px(window_dims.x));
  ui_next_height(ui_px(window_dims.y));
  state->root_box = ui_box_make(Str8{}, UI_Box_flag__NONE);

  state->mouse_pos_for_this_build   = mouse_pos;
  state->window_dims_for_this_build = window_dims;

  ui_push_parent(state->root_box);
}

void ui_end_build()
{
  ui_pop_parent();
  
  UI_State* state = ui_get_state();
  
  bool pointerDown = false; // TODO: Implement this
  Clay_SetPointerState({ state->mouse_pos_for_this_build.x, state->mouse_pos_for_this_build.y }, pointerDown);
  Clay_SetLayoutDimensions({ state->window_dims_for_this_build.x, state->window_dims_for_this_build.y });

  Clay_BeginLayout();
  __ui_build_clay_element_tree_from_box_tree(state->root_box);
  Clay_RenderCommandArray clay_render_commands = Clay_EndLayout();
  
  state->render_commands_as_result_of_ui_build = clay_render_commands; 
}

void __ui_build_clay_element_tree_from_box_tree(UI_Box* root)
{
  Clay__OpenElement();
  Clay__ConfigureOpenElementPtr(&root->clay_element_config);

  Str8 id = __ui_str8_from_clay_string(root->clay_element_config.id.stringId);
  if (str8_match(id, Str8FromC("Button id 2"), 0))
  {
    // BP;
  }

  for (UI_Box* child = root->first_child; child != 0 && child != &__ui_g_null_box; child = child->next_sibling)
  {
    __ui_build_clay_element_tree_from_box_tree(child);
  }

  Clay__CloseElement();
}

///////////////////////////////////////////////////////////
// - Box making
//
B32 ui_box_is_null(UI_Box* box)
{
  return (box == 0) || (box == &__ui_g_null_box);
}

// TODO:
// - static ids
// - dynamic ids
// - parent relative ids
// - indexed ids

UI_Box* ui_box_make(Str8 id, UI_Box_flags flags)
{
  UI_State* state = ui_get_state();
  
  UI_Box* new_box = ArenaPush(state->build_arena, UI_Box);
  *new_box = __ui_g_null_box;

  // Allocating the id and creating a hash for the box
  Clay_ElementId clay_id = {};
  if (id.count != 0)
  {
    Str8 box_id = str8_copy(state->build_arena, id);
    Clay_String clay_string_for_clay_id = __ui_clay_string_from_str8(box_id);
    clay_id = Clay__HashString(clay_string_for_clay_id, 0, 0);
  }

  __ui_get_next_box_clay_element_config(&new_box->clay_element_config, clay_id, flags);
  
  new_box->parent = ui_top_parent();
  if (!ui_box_is_null(new_box->parent))
  {
    DllPushBack_Name_NullFunc(new_box->parent, new_box, first_child, last_child, next_sibling, prev_sibling, ui_box_is_null);
    new_box->parent->children_count += 1;
  }

  // Auto popping all the stacks
  #define __UI_AUTO_POP_ALL_THE_STACKS(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, set_next_func_name, pop_func_name, auto_pop_func_name, get_top_func_name, stack_arr_capacity, defer_push_pop_macro_name) \
    auto_pop_func_name();
  __UI_STACK_DATA_TABLE_EXPANSION(__UI_AUTO_POP_ALL_THE_STACKS)
  #undef __UI_AUTO_POP_ALL_THE_STACKS

  return new_box;
}

UI_Box* ui_box_make_f(const char* fmt, UI_Box_flags flags, ...)
{
  Scratch scratch = get_scratch(0, 0);
  va_list args;
  va_start(args, flags);
  Str8 str = str8_valist(scratch.arena, fmt, args);
  UI_Box* box = ui_box_make(str, flags);
  va_end(args);
  end_scratch(&scratch);
  return box;
}

void __ui_get_next_box_clay_element_config(Clay_ElementDeclaration* config, Clay_ElementId clay_id, UI_Box_flags flags)
{
  config->id = clay_id;

  config->layout.sizing.width    = __ui_clay_sizing_axis_from_ui_size(ui_top_size_x());
  config->layout.sizing.height   = __ui_clay_sizing_axis_from_ui_size(ui_top_size_y());
  config->layout.layoutDirection = (ui_top_layout() == Axis2__x ?  CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM);
  
  if (flags & UI_Box_flag__has_padding) { config->layout.padding = __ui_clay_padding_from_v4f32(ui_top_padding()); }
  if (flags & UI_Box_flag__has_child_gap) { config->layout.childGap = (U16)ui_top_child_gap(); }
  config->layout.childAlignment  = {}; // TODO

  if (flags & UI_Box_flag__has_background) { config->backgroundColor = __ui_clay_color_from_v4f32(ui_top_background_color()); }
  if (flags & UI_Box_flag__has_rounded_corners) { config->cornerRadius = __ui_clay_corner_radius_from_v2f32(ui_top_corner_radius()); }

  if (flags & UI_Box_flag__clip_x) { config->clip.horizontal = true; }
  if (flags & UI_Box_flag__clip_y) { config->clip.vertical = true; }

  if (flags & UI_Box_flag__has_borders) { config->border = { __ui_clay_color_from_v4f32(ui_top_border_color()), __ui_clay_border_width_from_v4f32(ui_top_border_width()) }; }
  
  config->aspectRatio = {}; // TODO:
  
  config->image = {}; // TODO:
  config->floating = {}; // TODO;
  config->custom = {}; // TODO:

  // Damian: Not touching userData here, its for custom stuff, stuff like cutstom drawing
  // config->userData = {}; // TODO:
}

///////////////////////////////////////////////////////////
// - Box custom draw extention
//
void ui_extend_box_with_custom_draw_function(UI_Box* box, UI_Box_custom_draw_func_type* custom_draw, void* data) // TODO: Need a better name when you are sure what this does and is
{
  box->clay_element_config.custom.customData = (void*)custom_draw;
  box->clay_element_config.userData          = data;
}

// TODO: This is new test code, move it to a better place when done
// ========================================================
// TODO: See if these comments are still valid
struct UI_Actions {
  // Lower level actions
  B32 is_hovered;              // This is fine for all the boxes, id is not needed, no state is needed
  B32 is_down;                 // Cross frame state is needed, id to track if the box is the same between frames is needed
  B32 was_down;                // Cross frame state is needed, id to track if the box is the same between frames is needed
  B32 left_box_while_was_down; // Cross frame state is needed, id to track if the box is the same between frames is needed
  //
  // Composed for quick use
  B32 is_clicked; // These are composed, so we need cross frame state and id
  B32 went_down;  // These are composed, so we need cross frame state and id
  B32 went_up;    // These are composed, so we need cross frame state and id
};

UI_Actions ui_actions_from_box(UI_Box* box)
{
  if (box->has_been_updated_this_frame) { NotImplemented(); return {}; } 

  UI_State* state = ui_get_state();

  // Data to get
  B32 is_hovered              = false;
  B32 is_down                 = false;
  B32 was_down                = false;
  B32 left_box_while_was_down = false;
  B32 is_active               = false;
  B32 is_navigated            = false;

  is_hovered = Clay_PointerOver(box->clay_element_config.id); // TODO: See if this gets the most nested box or just checked if the mouse is inside the box's rect

  B32 some_other_box_is_being_interacted_with = (
    state->interacted_with_box_data.clay_id.id != 0 
    &&
    state->interacted_with_box_data.clay_id.id != box->clay_element_config.id.id
  );

  // Either there is no active box or we are the active box
  // Since interacted box data is retained across frame boundary, 
  // we just load the retained state and possibly update it here.
  // No need to load hover, we get it each frame just from the box rect.
  if (!some_other_box_is_being_interacted_with)
  {
    was_down                = state->interacted_with_box_data.is_mouse_down;
    left_box_while_was_down = state->interacted_with_box_data.did_mouse_leave_box_while_was_down;
  
    // Mouse is up, check if it goes down
    if (is_hovered && !was_down) 
    {
      // note: This has a bit of de sync relative to the is_hovered bool since we test if is hovered based on a different mouse pos than the one that was when the mouse went down, most of the time this shoud be fine, but i am not sure about the other times
      //       Might be nice to use mouse_pos from the prev frame or somethign like that, for now it should be fine
      B32 mouse_left_went_down = false;
      {
        OS_Event_list* events = os_get_frame_event_list();
        for (OS_Event* ev = events->first; ev; ev = ev->next)
        {
          if (ev->kind == OS_Event_kind__mouse && ev->mouse_event.button == Mouse_button__left && ev->mouse_event.went_down)
          {
            mouse_left_went_down = true;
            os_consume_frame_event(ev);
          }
        }
      }

      if (mouse_left_went_down)  
      {
        // We have a new interacted with box
        Assert(!was_down);
        Assert(!left_box_while_was_down);
        Assert(!state->interacted_with_box_data.is_mouse_down);
        Assert(!state->interacted_with_box_data.did_mouse_leave_box_while_was_down);
        Assert(state->interacted_with_box_data.clay_id.id == 0);

        is_down = true;
        state->interacted_with_box_data.is_mouse_down                      = true;
        state->interacted_with_box_data.did_mouse_leave_box_while_was_down = false;
        state->interacted_with_box_data.clay_id                            = box->clay_element_config.id;
      }
    }
    else if (was_down) 
    {
      is_down = true;

      if (!is_hovered && is_down) { 
        left_box_while_was_down = true; 
        state->interacted_with_box_data.did_mouse_leave_box_while_was_down = true;
      }

      // todo: The events api sucks right now, but it works, i will make a better one
      B32 mouse_left_went_up = false;
      {
        OS_Event_list* events = os_get_frame_event_list();
        for (OS_Event* ev = events->first; ev; ev = ev->next)
        {
          if (ev->kind == OS_Event_kind__mouse && ev->mouse_event.button == Mouse_button__left && ev->mouse_event.went_up)
          {
            mouse_left_went_up = true;
            os_consume_frame_event(ev);
            break;
          }
        }
      }

      if (mouse_left_went_up)
      {
        Assert(was_down);
        Assert(state->interacted_with_box_data.is_mouse_down);

        is_down = false;
        state->interacted_with_box_data.is_mouse_down                      = false;
        state->interacted_with_box_data.did_mouse_leave_box_while_was_down = false;
        state->interacted_with_box_data.clay_id                            = Clay_ElementId{};
      }
    }
  }

  // is_active    = str8_match(ctx->active_box_id, this_frames_box->id, 0);

  UI_Actions result_actions = {};

  result_actions.is_hovered              = is_hovered;            
  result_actions.is_down                 = is_down;               
  result_actions.was_down                = was_down;              
  result_actions.left_box_while_was_down = left_box_while_was_down;
  result_actions.is_clicked              = was_down && !is_down && !left_box_while_was_down;
  result_actions.went_down               = !was_down && is_down;
  result_actions.went_up                 = was_down && !is_down;  

  return result_actions;
}

///////////////////////////////////////////////////////////
// - UI drawing
//
void ui_draw()
{
  UI_State* state = ui_get_state();
  Clay_RenderCommandArray render_commands = state->render_commands_as_result_of_ui_build;

  d_push_scissor_rect(rect_make(0.0f, 0.0f, state->window_dims_for_this_build.x, state->window_dims_for_this_build.y));

  for EachIndex(commands_index, render_commands.length)
  {
    Clay_RenderCommand command = render_commands.internalArray[commands_index];
    switch (command.commandType)
    {
      case CLAY_RENDER_COMMAND_TYPE_NONE:
      default: { } break;

      case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
      {
        Rect rect          = __ui_rect_from_clay_bounding_box(command.boundingBox);
        V4F32 color        = __ui_v4f32_from_clay_color(command.renderData.rectangle.backgroundColor);
        V4F32 corner_radii = __ui_v4f32_from_clay_corner_radius(command.renderData.rectangle.cornerRadius);

        F32 softness = 0.0f; // Keeping softness as a var thought used only once for later search when we get to having softness used in rendering
        d_draw_rect_pro(rect, color, color, color, color, corner_radii, softness);
      } break;

      case CLAY_RENDER_COMMAND_TYPE_BORDER:
      {
        Rect rect          = __ui_rect_from_clay_bounding_box(command.boundingBox);
        V4F32 border_color = __ui_v4f32_from_clay_color(command.renderData.border.color);
        V4F32 corner_rs    = __ui_v4f32_from_clay_corner_radius(command.renderData.border.cornerRadius);
        V4F32 border_width = __ui_v4f32_from_clay_border_width(command.renderData.border.width);
        
        F32 softness = 0.0f; // Keeping softness as a var thought used only once for later search when we get to having softness used in rendering
        
        if (border_width.v[RectEdge__left] > 0) 
        {
          Rect left_border_rect = rect_make(rect.x, rect.y, border_width.v[RectEdge__left], rect.height);
          d_draw_rect_pro(left_border_rect, border_color, border_color, border_color, border_color, corner_rs, softness);
        }

        if (border_width.v[RectEdge__right] > 0)
        {
          Rect right_border_rect = rect_make(rect.x + rect.width - border_width.v[RectEdge__right], rect.y, border_width.v[RectEdge__right], rect.height);
          d_draw_rect_pro(right_border_rect, border_color, border_color, border_color, border_color, corner_rs, softness);
        }

        if (border_width.v[RectEdge__top] > 0)
        {
          Rect top_border_rect = rect_make(rect.x, rect.y, rect.width, border_width.v[RectEdge__top]);
          d_draw_rect_pro(top_border_rect, border_color, border_color, border_color, border_color, corner_rs, softness);
        }

        if (border_width.v[RectEdge__bottom] > 0)
        {
          Rect bottom_border_rect = rect_make(rect.x, rect.y + rect.height - border_width.v[RectEdge__bottom], rect.width, border_width.v[RectEdge__bottom]);
          d_draw_rect_pro(bottom_border_rect, border_color, border_color, border_color, border_color, corner_rs, softness);
        }
      } break;

      case CLAY_RENDER_COMMAND_TYPE_TEXT:
      {
        // Damian: Not sure if we need this yet
        NotImplemented();
      } break;

      case CLAY_RENDER_COMMAND_TYPE_IMAGE:
      {
        // Damian: Not sure if we need this yet
        NotImplemented();
      } break;

      case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
      {
        B32 is_axis_clipped[Axis2__COUNT] = { command.renderData.clip.horizontal, command.renderData.clip.vertical };
        Rect rect = __ui_rect_from_clay_bounding_box(command.boundingBox);
        
        Rect current_scissor_rect = __d_get_current_scissor_rect__defaults();
        if (is_axis_clipped[Axis2__x]) { current_scissor_rect = rect_intersect_on_axis(current_scissor_rect, rect, Axis2__x); }
        if (is_axis_clipped[Axis2__y]) { current_scissor_rect = rect_intersect_on_axis(current_scissor_rect, rect, Axis2__y); }
        d_push_scissor_rect(current_scissor_rect);
      } break;

      case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
      {
        d_pop_scissor_rect();
      } break;

      case CLAY_RENDER_COMMAND_TYPE_CUSTOM:
      {
        Rect rect           = __ui_rect_from_clay_bounding_box(command.boundingBox);
        V4F32 b_color       = __ui_v4f32_from_clay_color(command.renderData.custom.backgroundColor);
        V4F32 clay_corner_r = __ui_v4f32_from_clay_corner_radius(command.renderData.custom.cornerRadius);
        
        UI_Box_custom_draw_func_type* custom_draw_func = (UI_Box_custom_draw_func_type*)command.renderData.custom.customData;
        UI_Provided_data_for_custom_draw provided_data = {};
        provided_data.final_box_rect   = rect;
        provided_data.background_color = b_color;
        provided_data.corner_radii     = clay_corner_r;
        
        custom_draw_func(provided_data, command.userData);
      } break;
    }
  }

  d_pop_scissor_rect();
}


///////////////////////////////////////////////////////////
// - Size makers
//
UI_Size ui_size_make(UI_Size_kind kind, F32 value1, F32 value2)
{
  UI_Size size = {};
  size.kind   = kind;
  size.value1 = value1;
  size.value2 = value2; 
  return size;
}
UI_Size ui_px(F32 value)                 { return ui_size_make(UI_Size_kind__px, value, 0.0f); }
UI_Size ui_fit_mm(F32 min, F32 max)      { return ui_size_make(UI_Size_kind__fit, min, max); } 
UI_Size ui_grow_mm(F32 min, F32 max)     { return ui_size_make(UI_Size_kind__grow, min, max); }         
UI_Size ui_fit()                         { return ui_size_make(UI_Size_kind__fit, 0.0f, 0.0f); } 
UI_Size ui_grow()                        { return ui_size_make(UI_Size_kind__percent_of_parent, 0.0f, 0.0f); }         
UI_Size ui_p_of_p(F32 p)                 { return ui_size_make(UI_Size_kind__percent_of_parent, p, 0.0f); }         

///////////////////////////////////////////////////////////
// - Stack funtions
//
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DEFINE_PUSH_FUNC)
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DEFINE_SET_NEXT_FUNC)
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DEFINE_POP_FUNC)
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DEFINE_AUTO_POP_FUNC)
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DEFINE_TOP_FUNC)

V4F32 ui_top_padding()
{
  V4F32 padding = {};
  padding.v[0] = ui_top_padding_left();
  padding.v[1] = ui_top_padding_right();
  padding.v[2] = ui_top_padding_top();
  padding.v[3] = ui_top_padding_bottom();
  return padding;
}

V4F32 ui_top_corner_radius()
{
  V4F32 corner_r = {};
  corner_r.v[0] = ui_top_corner_radius_top_left();
  corner_r.v[1] = ui_top_corner_radius_top_right();
  corner_r.v[2] = ui_top_corner_radius_bottom_left();
  corner_r.v[3] = ui_top_corner_radius_bottom_right();
  return corner_r;
}

V4F32 ui_top_border_width()
{
  V4F32 border_width = {};
  border_width.v[0] = ui_top_border_left();
  border_width.v[1] = ui_top_border_right();
  border_width.v[2] = ui_top_border_top();
  border_width.v[3] = ui_top_border_bottom();
  return border_width;
}

void ui_next_width(UI_Size size) { ui_next_size_x(size); }
void ui_next_height(UI_Size size) { ui_next_size_y(size); }
void ui_next_b_color(V4F32 color) { ui_next_background_color(color); }
void ui_next_padding(F32 padding) 
{
  ui_next_padding_left(padding);
  ui_next_padding_right(padding);
  ui_next_padding_top(padding);
  ui_next_padding_bottom(padding);
}
void ui_next_corner_r(F32 r)
{
  ui_next_corner_radius_top_left(r);
  ui_next_corner_radius_top_right(r);
  ui_next_corner_radius_bottom_right(r);
  ui_next_corner_radius_bottom_left(r);
}
void ui_next_border_width(F32 border)
{
  ui_next_border_left(border);
  ui_next_border_right(border);
  ui_next_border_top(border);
  ui_next_border_bottom(border);
}
void ui_next_border(F32 width, V4F32 color)
{
  ui_next_border_width(width);
  ui_next_border_color(color);
}

///////////////////////////////////////////////////////////
// - Helpers to wrap around clay
//
Clay_SizingAxis __ui_clay_sizing_axis_from_ui_size(UI_Size ui_size)
{
  Clay_SizingAxis clay_size = {};
  
  if (0) {}
  else if (ui_size.kind == UI_Size_kind__px) 
  { 
    clay_size.type = CLAY__SIZING_TYPE_FIXED; 
    clay_size.size.minMax.min = ui_size.value1;
    clay_size.size.minMax.max = ui_size.value1;
    Assert(ui_size.value2 == 0.0f);
  }
  else if (ui_size.kind == UI_Size_kind__fit)               
  { 
    clay_size.type = CLAY__SIZING_TYPE_FIT; 
    clay_size.size.minMax.min = ui_size.value1;
    clay_size.size.minMax.max = ui_size.value2;
  }
  else if (ui_size.kind == UI_Size_kind__percent_of_parent) 
  { 
    clay_size.type = CLAY__SIZING_TYPE_PERCENT; 
    clay_size.size.percent = ui_size.value1;
    Assert(ui_size.value1 == ui_size.value2);
  }
  else if (ui_size.kind == UI_Size_kind__grow)              
  { 
    clay_size.type = CLAY__SIZING_TYPE_GROW; 
    clay_size.size.minMax.min = ui_size.value1;
    clay_size.size.minMax.max = ui_size.value2;
  }

  return clay_size;
}

Clay_Padding __ui_clay_padding_from_v4f32(V4F32 padding)
{
  Clay_Padding clay_padding = {};
  clay_padding.left   = (U16)padding.v[0];
  clay_padding.right  = (U16)padding.v[1];
  clay_padding.top    = (U16)padding.v[2];
  clay_padding.bottom = (U16)padding.v[3];
  return clay_padding;
}

Clay_Color __ui_clay_color_from_v4f32(V4F32 color)
{
  Clay_Color clay_color = {};
  clay_color.r = color.r;
  clay_color.g = color.g;
  clay_color.b = color.b;
  clay_color.a = color.a;
  return clay_color;
}

V4F32 __ui_v4f32_from_clay_color(Clay_Color clay_color)
{
  V4F32 color = {};
  color.r = clay_color.r;
  color.g = clay_color.g;
  color.b = clay_color.b;
  color.a = clay_color.a;
  return color;
}

Clay_BorderWidth __ui_clay_border_width_from_v4f32(V4F32 border)
{
  Clay_BorderWidth clay_border = {};
  clay_border.left            = (U16)border.v[RectEdge__left];
  clay_border.right           = (U16)border.v[RectEdge__right];
  clay_border.top             = (U16)border.v[RectEdge__top];
  clay_border.bottom          = (U16)border.v[RectEdge__bottom];
  clay_border.betweenChildren = {}; // Not sure if we need this, so not using this yet
  return clay_border;
}

V4F32 __ui_v4f32_from_clay_border_width(Clay_BorderWidth clay_border_width)
{
  V4F32 vec = {};
  vec.v[RectEdge__left]   = clay_border_width.left;
  vec.v[RectEdge__right]  = clay_border_width.right;
  vec.v[RectEdge__top]    = clay_border_width.top;
  vec.v[RectEdge__bottom] = clay_border_width.bottom;
  return vec;
}

Str8 __ui_str8_from_clay_string(Clay_String clay_string)
{
  Str8 str = {};
  str.data  = (U8*)clay_string.chars;
  str.count = (U64)clay_string.length;
  return str;
}

Clay_String __ui_clay_string_from_str8(Str8 str)
{
  Clay_String clay_str = {};
  clay_str.isStaticallyAllocated = false;
  clay_str.length                = (U32)str.count; Assert(str.count <= u32_max); // TODO: What do we do about that
  clay_str.chars                 = (char*)str.data;
  return clay_str;
}

Rect __ui_rect_from_clay_bounding_box(Clay_BoundingBox bbox)
{
  Rect rect = {};
  rect.x      = bbox.x;
  rect.y      = bbox.y;
  rect.width  = bbox.width;
  rect.height = bbox.height;
  return rect;
}

Clay_BoundingBox __ui_clay_bounding_box_from_rect(Rect rect)
{
  Clay_BoundingBox bbox = {};
  bbox.x      = rect.x;
  bbox.y      = rect.y;
  bbox.width  = rect.width;
  bbox.height = rect.height;
  return bbox;
}

V4F32 __ui_v4f32_from_clay_corner_radius(Clay_CornerRadius clay_crs)
{
  V4F32 vec = {};
  vec.v[UV__top_left]     = clay_crs.topLeft;
  vec.v[UV__top_right]    = clay_crs.topRight;
  vec.v[UV__bottom_left]  = clay_crs.bottomLeft;
  vec.v[UV__bottom_right] = clay_crs.bottomRight;
  return vec;
}

Clay_CornerRadius __ui_clay_corner_radius_from_v2f32(V4F32 vec)
{
  Clay_CornerRadius clay_crs = {};
  clay_crs.topLeft     = vec.v[UV__top_left];
  clay_crs.topRight    = vec.v[UV__top_right];
  clay_crs.bottomLeft  = vec.v[UV__bottom_left];
  clay_crs.bottomRight = vec.v[UV__bottom_right];
  return clay_crs;
}

#endif











