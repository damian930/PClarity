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
  Assert( __ui_g_null_box)
  
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
  
  // TODO: Clay_SetPointerState;
  // TODO: Clay_SetLayoutDimensions

  Clay_BeginLayout();
  __ui_build_clay_element_tree_from_box_tree(state->root_box);
  Clay_RenderCommandArray clay_render_commands = Clay_EndLayout();
  
  state->render_commands_as_result_of_ui_build = clay_render_commands; 
}

void __ui_build_clay_element_tree_from_box_tree(UI_Box* root)
{
  Clay__OpenElement();
  Clay__ConfigureOpenElementPtr(&root->clay_element_config);

  for (UI_Box* child = root->first_child; child != 0 && child != &__ui_g_null_box; child = child->next_sibling)
  {
    __ui_build_clay_element_tree_from_box_tree(child);
  }

  Clay__CloseElement();
}

// // Begin layout
// Clay_BeginLayout();

// // Your UI goes here
// BuildUI();

// // Finish layout
// Clay_RenderCommandArray commands = Clay_EndLayout();

// // Draw the commands using your renderer
// Render(commands);

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

UI_Box* ui_box_make(Str8 id_and_text, UI_Box_flags flags)
{
  Clay_String key = ...;
  Clay_ElementId id = Clay__HashString(Clay_String key, 0, 0);


  
  // - get id from id_and_text
  // - get text from id_and_text
  // - make a clay string from key
  // - generate hash from it
  // - use that hash

  UI_State* state = ui_get_state();
  
  UI_Box* new_box = ArenaPush(state->build_arena, UI_Box);
  *new_box = __ui_g_null_box;

  __ui_get_next_box_clay_element_config(&new_box->clay_element_config, flags);
  
  new_box->parent = ui_top_parent();
  if (!ui_box_is_null(new_box->parent))
  {
    DllPushBack_Name_NullFunc(new_box->parent, new_box, first_child, last_child, next_sibling, prev_sibling, ui_box_is_null);
    new_box->parent->children_count += 1;
  }

  return new_box;
}

// TODO:
// void ui_box_make_f(const char* fmt, UI_Box_flags flags, ...)
// {
//   Scratch scratch = get_scratch(0, 0);
//   va_list args;
//   va_start(args, flags);
//   Str8 str = str8_valist(scratch.arena, fmt, args);
//   UI_Box* box = ui_box_make(str, flags);
//   va_end(args);
//   end_scratch(&scratch);
//   return box;
// }

void __ui_get_next_box_clay_element_config(Clay_ElementDeclaration* config, UI_Box_flags flags)
{
  config->id = {}; // TODO

  config->layout.sizing.width    = __ui_clay_sizing_axis_from_ui_size(ui_top_size_x());
  config->layout.sizing.height   = __ui_clay_sizing_axis_from_ui_size(ui_top_size_y());
  config->layout.layoutDirection = (ui_top_layout() == Axis2__x ?  CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM);
  
  if (flags & UI_Box_flag__has_padding) { config->layout.padding = __ui_clay_padding_from_v4f32(ui_top_padding()); }
  if (flags & UI_Box_flag__has_child_gap) { config->layout.childGap = (U16)ui_top_child_gap(); }
  config->layout.childAlignment  = {}; // TODO

  if (flags & UI_Box_flag__has_background) { config->backgroundColor = __ui_clay_color_from_v4f32(ui_top_background_color()); }
  if (flags & UI_Box_flag__has_rounded_corners) { config->backgroundColor = { ui_top_corner_radius().v[UV__top_left], ui_top_corner_radius().v[UV__top_right], ui_top_corner_radius().v[UV__bottom_left], ui_top_corner_radius().v[UV__bottom_right] }; }

  if (flags & UI_Box_flag__clip_x) { config->clip.horizontal = true; }
  if (flags & UI_Box_flag__clip_y) { config->clip.vertical = true; }

  if (flags & UI_Box_flag__has_borders) { config->border = { __ui_clay_color_from_v4f32(ui_top_border_color()), __ui_clay_border_width_from_v4f32(ui_top_border_width()) }; }
  
  config->aspectRatio = {}; // TODO:
  
  config->image = {}; // TODO:
  config->floating = {}; // TODO;
  config->custom = {}; // TODO:

  config->userData = {}; // TODO:
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
    Clay_RenderCommand command     = render_commands.internalArray[commands_index];
    Clay_BoundingBox clay_box_rect = command.boundingBox;
    switch (command.commandType)
    {
      case CLAY_RENDER_COMMAND_TYPE_NONE:
      default: { } break;

      case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
      {
        Clay_Color clay_box_b_color         = command.renderData.rectangle.backgroundColor;
        Clay_CornerRadius clay_box_corner_r = command.renderData.rectangle.cornerRadius;

        Rect rect          = {};
        V4F32 color        = {};
        V4F32 corner_radii = {};
        MemCopySafe(rect, clay_box_rect); 
        MemCopySafe(color, clay_box_b_color); 
        MemCopySafe(corner_radii, clay_box_corner_r);

        F32 softness = 0.0f; // Keeping softness as a var thought used only once for later search when we get to having softness used in rendering
        d_draw_rect_pro(rect, color, color, color, color, corner_radii, softness);
      } break;

      case CLAY_RENDER_COMMAND_TYPE_BORDER:
      {
        Clay_Color clay_border_color       = command.renderData.border.color;
        Clay_CornerRadius clay_corner_r    = command.renderData.border.cornerRadius;
        Clay_BorderWidth clay_border_width = command.renderData.border.width;
        
        Rect rect          = {};
        V4F32 color        = {};
        V4F32 corner_radii = {};
        MemCopySafe(rect, clay_box_rect); 
        MemCopySafe(color, clay_border_color); 
        MemCopySafe(corner_radii, clay_corner_r);

        F32 softness = 0.0f; // Keeping softness as a var thought used only once for later search when we get to having softness used in rendering
        if (clay_border_width.left > 0) 
        {
          Rect left_border_rect = rect_make(rect.x, rect.y, clay_border_width.left, rect.height);
          d_draw_rect_pro(left_border_rect, color, color, color, color, corner_radii, softness);
        }

        if (clay_border_width.right > 0) 
        {
          Rect right_border_rect = rect_make(rect.x + rect.width - clay_border_width.right, rect.y, clay_border_width.right, rect.height);
          d_draw_rect_pro(right_border_rect, color, color, color, color, corner_radii, softness);
        }

        if (clay_border_width.top > 0) 
        {
          Rect top_border_rect = rect_make(rect.x, rect.y, rect.width, clay_border_width.top);
          d_draw_rect_pro(top_border_rect, color, color, color, color, corner_radii, softness);
        }

        if (clay_border_width.bottom > 0) 
        {
          Rect bottom_border_rect = rect_make(rect.x, rect.y + rect.height - clay_border_width.bottom, rect.width, clay_border_width.bottom);
          d_draw_rect_pro(bottom_border_rect, color, color, color, color, corner_radii, softness);
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
        Rect clip_rect = {};
        MemCopySafe(clip_rect, clay_box_rect); 
        
        Rect current_scissor_rect = __d_get_current_scissor_rect__defaults();
        if (is_axis_clipped[Axis2__x]) { current_scissor_rect = rect_intersect_on_axis(current_scissor_rect, clip_rect, Axis2__x); }
        if (is_axis_clipped[Axis2__y]) { current_scissor_rect = rect_intersect_on_axis(current_scissor_rect, clip_rect, Axis2__y); }
        d_push_scissor_rect(current_scissor_rect);
      } break;

      case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
      {
        d_pop_scissor_rect();
      } break;

      case CLAY_RENDER_COMMAND_TYPE_CUSTOM:
      {
        NotImplemented();
      } break;
    }
  }

  d_pop_scissor_rect();
}

// TODO: Move this to the bottom when done

// F32 ui_get_mouse_x() { UI_State* ctx = ui_get_state(); return ctx->mouse_x;  }
// F32 ui_get_mouse_y() { UI_State* ctx = ui_get_state(); return ctx->mouse_y;  }
// V2F32 ui_get_mouse_pos() { return v2f32(ui_get_mouse_x(), ui_get_mouse_y()); }

// // todo: Have a way to handle erros in clay
// void errorHandlerFunction(Clay_ErrorData errorText) {  }

// UI_Box* ui_box_make(Str8 id_and_text, UI_Box_flags flags)
// {
//   // TODO: Start the clay thing here, then push the parent

//   // TODO: ID and all the styling here as well
  
//   Clay_LayoutConfig clay_box_layout = {};
//   clay_box_layout.sizing          = { __ui_clay_sizing_axis_from_ui_size(ui_top_size_x()), __ui_clay_sizing_axis_from_ui_size(ui_top_size_y()) }; 
//   clay_box_layout.padding         = CLAY_PADDING_ALL(); // TODO
//   clay_box_layout.childGap        = value; // TODO
//   clay_box_layout.childAlignment  = {}; // TODO
//   clay_box_layout.layoutDirection = {}; // TODO


  
//   Clay_ElementDeclaration clay_element_decl = {};
//   clay_element_decl.id              = {}; // TODO:
//   clay_element_decl.layout          = {}; // TODO:
//   clay_element_decl.backgroundColor = {}; // TODO:
//   clay_element_decl.cornerRadius    = {}; // TODO:
//   clay_element_decl.aspectRatio     = {}; // TODO:
//   clay_element_decl.image           = {}; // TODO:
//   clay_element_decl.floating        = {}; // TODO:
//   clay_element_decl.custom          = {}; // TODO:
//   clay_element_decl.clip            = {}; // TODO:
//   clay_element_decl.border          = {}; // TODO:
//   clay_element_decl.userData        = {}; // TODO:

//   Clay__OpenElement();
//   Clay__ConfigureOpenElementPtr(&clay_element_decl);




//   // TODO: Push parent in clay if it doent do that automatically
//   // TODO: Here will be the children for clay stuff

//   // Here we end the thing 

//   CLAY(CLAY_ID("Root"), {
//     .layout = { .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
//                 .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER } },
//     .backgroundColor = COLOR_BG
//   }) {
//       CLAY(CLAY_ID("SimpleBox"), {
//           .layout = { .sizing = { CLAY_SIZING_FIXED(200), CLAY_SIZING_FIXED(100) } },
//           .backgroundColor = COLOR_BOX,
//           .cornerRadius = CLAY_CORNER_RADIUS(12)
//       }) {}
//   }

//   UI_State* ctx = ui_get_state();
//   Arena* arena = ui_get_build_arena();
  
//   UI_Box* box = ArenaPush(arena, UI_Box);
//   box->id = str8_copy(ui_get_build_arena(), id_and_text);
  
//   flags |= ui_get_flags(); 
//   box->flags                   = flags;
//   box->layout_axis             = ui_get_layout_axis();    
//   box->semantic_size[Axis2__x] = ui_get_size_x();        
//   box->semantic_size[Axis2__y] = ui_get_size_y();        
  
//   {
//     V4F32 border_color = ui_get_border_color();
//     F32 border_width   = ui_get_border_width();
//     if (!(flags & UI_Box_flag__has_borders)) { 
//       border_color = ui_get_border_color();
//       border_width = ui_get_border_width();
//     }
//     box->border_color = border_color;
//     box->border_width = border_width;
//   }

//   {
//     for EachEnumRange(i, UV, UV__x0y0, UV__COUNT) {
//       V4F32 vertex_color = ui_get_b_color_uv(i);
//       if (!(flags & UI_Box_flag__has_background)) {
//         vertex_color = ctx->defaults.vertex_colors[i];
//       }
//       box->vertex_colors[i] = vertex_color;
//     }
//   }

//   {
//     V4F32 corner_r = ui_get_corner_r();
//     if (!(flags & UI_Box_flag__has_rounded_corners)) { corner_r = ctx->defaults.corner_radii; }
//     box->corner_radii = corner_r;
//   }

//   box->softness = ui_get_softness();

//   {
//     FP_Font font = ctx->defaults.font; 
//     if (flags & UI_Box_flag__has_text_contents)
//     {
//       box->text = str8_copy(ui_get_build_arena(), ui_get_text_part_from_str8(id_and_text));
//       font = ui_get_font();
//     }
//     box->font = font;
//   }

//   DllPushBack_Name_NullFunc(ctx->current_parent_box, box, first_child, last_child, next_sibling, prev_sibling, ui_box_is_zero);
//   box->parent = ui_get_parent();
//   box->parent->children_count += 1;
//   box->first_child  = &__ui_g_zero_box;
//   box->last_child   = &__ui_g_zero_box;
//   box->next_sibling = &__ui_g_zero_box;
//   box->prev_sibling = &__ui_g_zero_box;

//   // Resetting possible single use valus on the style stacks
//   ui_pop_single_usage_flags();
//   ui_pop_single_usage_layout_axis();
//   ui_pop_single_usage_size_x();
//   ui_pop_single_usage_size_y();
//   ui_pop_single_usage_padding();
//   ui_pop_single_usage_b_color();
//   ui_pop_single_usage_corner_r();
//   ui_pop_single_usage_softness();
//   ui_pop_single_usage_font();

//   return box;
// }

// UI_Box* ui_box_make_f(const char* fmt, UI_Box_flags flags, ...)
// {
//   Scratch scratch = get_scratch(0, 0);
//   va_list args;
//   va_start(args, flags);
//   Str8 str = str8_valist(scratch.arena, fmt, args);
//   UI_Box* box = ui_box_make(str, flags);
//   va_end(args);
//   end_scratch(&scratch);
//   return box;
// }

// void ui_box_set_custom_draw(UI_Box* box, void (*draw_func) (UI_Box*), void* data)
// {
//   box->custom_draw_func = draw_func; 
//   box->custom_draw_data = data; 
// }

// void ui_push_parent(UI_Box* box)
// {
//   UI_State* ctx = ui_get_state();
//   box->parent = ctx->current_parent_box; 
//   ctx->current_parent_box = box;
// }

// void ui_pop_parent()
// {
//   UI_State* ctx = ui_get_state();
//   ctx->current_parent_box = ctx->current_parent_box->parent;
// }

// UI_Box* ui_get_parent()
// {
//   return ui_get_state()->current_parent_box;
// }

// // note: This was used to find the most inner child that is hovered to set hot_box when we had hot boxes
// // 
// // UI_Box* find_hoverd_child_for_box(UI_Box* box)
// // {
// //   if (ui_box_is_zero(box)) { return &__ui_g_zero_box; }

// //   // Hover is on this substree
// //   UI_Box* result_box = &__ui_g_zero_box;
// //   if (range_v2f32_within(box->final_on_screen_bbox, ui_get_mouse_pos()))
// //   {
// //     if (box->id.count != 0 && box->flags & UI_Box_flag__hoverable) { result_box = box; }
// //     for (UI_Box* child = box->first_child; !ui_box_is_zero(child); child = child->next_sibling)
// //     {
// //       UI_Box* hovered_box_inside_children = find_hoverd_child_for_box(child);
// //       if (!ui_box_is_zero(hovered_box_inside_children)) { result_box = hovered_box_inside_children; }
// //     }
// //   }

// //   return result_box;
// // }

// UI_Box* next_box_in_subtree_depth_first(UI_Box* start_box, B32 include_start_box)
// {
//   // Going over the sub-tree that start with start_box at the root
//   if (ui_box_is_zero(start_box)) { return &__ui_g_zero_box; }
//   if (start_box->id.count != 0 && include_start_box) { return start_box; }
//   for (UI_Box* child = start_box->first_child; !ui_box_is_zero(child); child = child->next_sibling)
//   {
//     if (child->id.count != 0) { return child; }
//     UI_Box* possible_box = next_box_in_subtree_depth_first(child, false);
//     if (possible_box->id.count != 0) { return possible_box; }
//   }
//   return &__ui_g_zero_box;
// }

// UI_Box* need_a_name(UI_Box* start_box)
// {
//   if (ui_box_is_zero(start_box)) { return &__ui_g_zero_box; }
  
//   UI_Box* test_box = next_box_in_subtree_depth_first(start_box, false);
//   if (!ui_box_is_zero(test_box)) { return test_box; }

//   for (UI_Box* sibling = start_box->next_sibling; !ui_box_is_zero(sibling); sibling = sibling->next_sibling)
//   {
//     test_box = next_box_in_subtree_depth_first(sibling, true);
//     if (!ui_box_is_zero(test_box)) { return test_box; }
//   }

//   return need_a_name(start_box->parent->next_sibling);
// }

// ///////////////////////////////////////////////////////////
// // - Layout algorithm
// //
// // ========
// // | algo:
// // | - figure out sizes for each box
// // |   - size fixed sized boxes
// // |   - % of parent
// // |   - size children dependant boxes
// // |   - layout fixing 
// // | - position each box, this is just relative to its parent
// // | - create final bounding boxes 

// void __ui_do_sizing_for_fixed_sized_box(UI_Box* root, Axis2 axis)
// {
//   switch (root->semantic_size[axis].kind)
//   {
//     default: {} break;

//     case UI_Size_kind__px:
//     {
//       root->final_on_screen_size.v[axis] = root->semantic_size[axis].value;
//     } break;

//     case UI_Size_kind__text:
//     {
//       V2F32 dims = fp_measure_text(root->text, root->font);
//       root->final_on_screen_size.v[axis] = dims.v[axis];
//     } break;
//   }
  
//   for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling) {
//     __ui_do_sizing_for_fixed_sized_box(child, axis);
//   }
// }

// void __ui_do_sizing_for_parent_dependant_box(UI_Box* root, Axis2 axis)
// {
//   if (root->semantic_size[axis].kind == UI_Size_kind__percent_of_parent)
//   {
//     UI_Box* first_non_child_dependant_parent = &__ui_g_zero_box;
//     for (UI_Box* ancestor = root->parent; !ui_box_is_zero(ancestor); ancestor = ancestor->parent) {
//       if (ancestor->semantic_size[axis].kind != UI_Size_kind__fit) {
//         first_non_child_dependant_parent = ancestor;
//         break; 
//       }
//     }
//     Assert(!ui_box_is_zero(first_non_child_dependant_parent)); // The top ui box is fixed sized, so we shoud get this allways

//     F32 parent_size = first_non_child_dependant_parent->final_on_screen_size.v[axis];
//     clamp_f32_inplace(&root->semantic_size[axis].value, 0.0f, 1.0f);
//     root->final_on_screen_size.v[axis] = parent_size * root->semantic_size[axis].value;
//   }
  
//   for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling) {
//     __ui_do_sizing_for_parent_dependant_box(child, axis);
//   }
// }

// void __ui_do_sizing_for_children_dependant_box(UI_Box* root, Axis2 axis)
// {
//   for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling) {
//     __ui_do_sizing_for_children_dependant_box(child, axis);
//   }

//   if (root->semantic_size[axis].kind != UI_Size_kind__fit) { return; }
  
//   for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
//   {
//     if (!(child->flags & UI_Box_flag__floating_x<<axis))
//     {
//       if (root->layout_axis == axis) { root->final_on_screen_size.v[axis] += child->final_on_screen_size.v[axis]; }
//       else { root->final_on_screen_size.v[axis] = Max(root->final_on_screen_size.v[axis], child->final_on_screen_size.v[axis]); }
//     }
//   }
  
//   // Dynamically calculating strictness based on already calculated children
//   if (root->layout_axis == axis) 
//   {
//     F32 children_size_to_maybe_give_out = 0.0f;
//     for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling) 
//     {
//       if (child->flags & UI_Box_flag__floating_x<<axis) { continue; }

//       F32 child_size                  =  child->final_on_screen_size.v[axis];
//       F32 p_to_to_keep                =  child->semantic_size[axis].strictness;
//       F32 p_to_give_out               =  1.0f - p_to_to_keep;
//       F32 size_to_give_out            =  child_size * p_to_give_out;
//       children_size_to_maybe_give_out += size_to_give_out;
//     }
//     F32 root_size = root->final_on_screen_size.v[axis];
//     if (root_size != 0.0f)
//     {
//       F32 root_p_to_give_out = children_size_to_maybe_give_out / root_size;
//       root->semantic_size[axis].strictness = 1.0f - root_p_to_give_out;
//     }
//   }
//   else 
//   {
//     F32 max_size_after_possible_fixing = 0.0f;
//     for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling) 
//     {
//       if (child->flags & UI_Box_flag__floating_x<<axis) { continue; }

//       F32 child_size                = child->final_on_screen_size.v[axis];
//       F32 p_to_to_keep              = child->semantic_size[axis].strictness;
//       F32 p_to_give_out             = 1.0f - p_to_to_keep;
//       F32 size_to_give_out          = child_size * p_to_give_out;
//       F32 child_size_after_give_out = child_size - size_to_give_out;
//       max_size_after_possible_fixing = Max(max_size_after_possible_fixing, child_size_after_give_out);
//     }

//     F32 root_size = root->final_on_screen_size.v[axis];
//     if (root_size != 0.0f)
//     {
//       F32 root_p_to_keep = max_size_after_possible_fixing / root_size;
//       root->semantic_size[axis].strictness = root_p_to_keep;
//     }
//   }
// }

// void __ui_do_layout_size_fixing(UI_Box* root, Axis2 axis)
// {
//   // if (axis == Axis2__y)
//   // if (str8_match(root->id, Str8FromC("Test id"), 0)) 
//   // { BP; }

//   // Testing this codepath for floating with p of p size
//   if ((root->flags & UI_Box_flag__floating_x<<axis) && root->semantic_size[axis].kind == UI_Size_kind__percent_of_parent)
//   {
//     F32 new_size = root->parent->final_on_screen_size.v[axis] * root->semantic_size[axis].value;
//     root->final_on_screen_size.v[axis] = new_size;
//   }

//   // If the inner contents of the root are larger then the root alowes, 
//   // then we make the smaller here based on the strictness if possible.

//   if (root->layout_axis == axis)
//   {
//     F32 total_size_of_children  = 0.0f;
//     F32 strictness_total_budget = 0.0f;
//     for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
//     {
//       // if (str8_match(child->id, Str8FromC("Test"), 0)) { BP; }

//       if (child->flags & UI_Box_flag__floating_x<<axis) { continue; }

//       total_size_of_children += child->final_on_screen_size.v[axis];
//       strictness_total_budget += (1.0f - child->semantic_size[axis].strictness);
//     }
  
//     if (strictness_total_budget > 0.0f) 
//     {
//       // We have overflow of children here, might need fixing
//       F32 root_size = root->final_on_screen_size.v[axis];
//       if (root_size < total_size_of_children)
//       {
//         for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
//         {
//           if (child->flags & UI_Box_flag__floating_x<<axis) { continue; }

//           F32 percentage_of_removable_size    =  1.0f - child->semantic_size[axis].strictness;
//           F32 ratio_relative_to_total_budget  =  percentage_of_removable_size / strictness_total_budget;
//           F32 amount_to_remove                =  (total_size_of_children - root_size) * ratio_relative_to_total_budget;
//           child->final_on_screen_size.v[axis] -= amount_to_remove;
//         }
//       }
//     }
//   }
//   else {
//     for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
//     {
//       if (child->flags & UI_Box_flag__floating_x<<axis) { continue; }

//       F32 child_max_legal_size = root->final_on_screen_size.v[axis];
//       F32 child_size = child->final_on_screen_size.v[axis];
//       if (child_size > child_max_legal_size)
//       {
//         F32 removable_budget  = 1.0f - child->semantic_size[axis].strictness;
//         F32 overflow          = child_size - child_max_legal_size;
//         F32 possible_new_size = child->final_on_screen_size.v[axis] - overflow;
//         F32 legal_min_size    = child->final_on_screen_size.v[axis] * child->semantic_size[axis].strictness;
//         child->final_on_screen_size.v[axis] = Max(legal_min_size, possible_new_size);
//       }
//     }
//   }

  
//   for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
//   {
//     __ui_do_layout_size_fixing(child, axis);
//   }
// }

// void __ui_do_relative_parent_offsets_for_box(UI_Box* root, Axis2 axis)
// {
//   U64 child_index = 0;
//   F32 accumelated_children_sizes = 0.0f;
//   for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling, child_index += 1)
//   {
//     // Floating doesnt have offset from parent
//     if (child->flags & UI_Box_flag__floating_x<<axis) {}
//     else 
//     {
//       // NOTE: It seems like all the rest of the bottom
//       //       and top centering would be done this way here 
//       //       for the non layout axis.
//       //       Right now i have no idea about the layout
//       //       axis.
//       if (axis == root->layout_axis) {
//         child->final_parent_offset.v[axis] += accumelated_children_sizes;
//         accumelated_children_sizes += child->final_on_screen_size.v[axis];
//       }
//       else if (axis != root->layout_axis && root->center_children_on_non_layout_axis)
//       {
//         F32 space_left_in_parent_after_child = root->final_on_screen_size.v[axis] - child->final_on_screen_size.v[axis];
//         child->final_parent_offset.v[axis] = (space_left_in_parent_after_child / 2.0f);
//       }
//     }

//     // Clip aplies on everything, even floating
//     child->final_parent_offset.v[axis] += root->clip_offset.v[axis];

//     __ui_do_relative_parent_offsets_for_box(child, axis);
//   }
// }

// void __ui_do_final_rect_for_box(UI_Box* root, Axis2 axis, RangeV2F32 parent_clip_bbox)
// {
//   static F32 total_offset[Axis2__COUNT] = {};

//   // Positioning boxes regardless of clip
//   root->final_on_screen_bbox.min.v[axis] = total_offset[axis] + root->final_parent_offset.v[axis];
//   root->final_on_screen_bbox.max.v[axis] = root->final_on_screen_bbox.min.v[axis] + root->final_on_screen_size.v[axis];

//   // Dealing with clip rects
//   root->clip_bbox.min.v[axis] = parent_clip_bbox.min.v[axis]; 
//   root->clip_bbox.max.v[axis] = parent_clip_bbox.max.v[axis]; 
//   //
//   RangeV2F32 new_clip_bbox = parent_clip_bbox;
//   {
//     if (root->flags & UI_Box_flag__clip_x<<axis) {
//       new_clip_bbox = intersect_range_v2f32_on_axis(parent_clip_bbox, root->final_on_screen_bbox, axis);
//     }
//   }

//   // Doing children
//   F32 children_size_sum = 0.0f;
//   U64 child_index = 0;
//   for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling, child_index += 1)
//   {
//     if (root->layout_axis == axis) { children_size_sum += child->final_on_screen_size.v[axis]; }
//     else { children_size_sum = Max(children_size_sum, child->final_on_screen_size.v[axis]); }

//     F32 prev_total_offset = total_offset[axis]; 
//     total_offset[axis] = root->final_on_screen_bbox.min.v[axis];
//     __ui_do_final_rect_for_box(child, axis, new_clip_bbox);
//     total_offset[axis] = prev_total_offset;
//   }
//   root->inner_content_dims.v[axis] = children_size_sum;
// }

// void __ui_layout_box(UI_Box* root, Axis2 axis)
// { 
//   // Sizing
//   __ui_do_sizing_for_fixed_sized_box(root, axis);     
//   __ui_do_sizing_for_parent_dependant_box(root, axis); 
//   __ui_do_sizing_for_children_dependant_box(root, axis);      
  
//   // Fixing
//   __ui_do_layout_size_fixing(root, axis);

//   // Final positioning
//   __ui_do_relative_parent_offsets_for_box(root, axis);

//   // TODO: 1000 is a bit too little here
//   RangeV2F32 parent_clip_bbox = range_v2f32(v2f32(-1000.0f, -1000.0f), v2f32(1000.0f, 1000.0f));
//   __ui_do_final_rect_for_box(root, axis, parent_clip_bbox);
// }

// ///////////////////////////////////////////////////////////
// // - Box data stuff
// //

// // note: There might be weird thing going on with ids and text, dont forget about ##
// UI_Box* ui_get_box_from_tree(UI_Box* root, Str8 id)
// {
//   if (id.count == 0)               { return &__ui_g_zero_box; }
//   if (ui_box_is_zero(root))        { return &__ui_g_zero_box; }
//   if (str8_match(root->id, id, 0)) { return root; }
  
//   for (UI_Box* child = root->first_child; !ui_box_is_zero(child); child = child->next_sibling)
//   {
//     // note: I am not sure, but this shoud be faster
//     //       since ui boxed are allocated in depth order
//     if (str8_match(child->id, id, 0)) { return child; }
//     else { 
//       UI_Box* box = ui_get_box_from_tree(child, id); 
//       if (!ui_box_is_zero(box)) { return box; }
//     }
//   }
//   return &__ui_g_zero_box;
// }

// UI_Box* ui_get_box_prev_frame(Str8 id)
// {
//   UI_State* ctx = ui_get_state();
//   UI_Box* box = ui_get_box_from_tree(ctx->prev_frame_root_box, id);
//   return box;
// }

// UI_Box_data ui_box_data_from_box_prev_frame(UI_Box* box)
// {
//   return ui_box_data_from_box_id_prev_frame(box->id);
// }

// UI_Box_data ui_box_data_from_box_id_prev_frame(Str8 id)
// {
//   UI_Box_data box_data = {};
//   UI_Box* box = ui_get_box_prev_frame(id);
//   if (!ui_box_is_zero(box)) 
//   { 
//     box_data.is_found           = true; 
//     box_data.on_screen_bbox     = box->final_on_screen_bbox; 
//     box_data.inner_content_dims = box->inner_content_dims;
//     box_data.clip_offset        = box->clip_offset;
//   }
//   return box_data;
// }

// /* IDEAS ABOUT ACTIONS FOR POSSIBLE LATER:
//   - Right now hover and active work very simply. The caller just asks for the events,
//     the call then checks the boxe's rect and does hover and if mosue is down the active logic.
//     This is not the best way to do this. 
//     Here are some cases when this fais:
//       A box with another box that has id and when we press inner box we would like the outer box to be active.
//       This cant be done, since the inner box will get active and we dont have a way to bubble or propogate.
//       ---
//       A box with inner box and we do hover effect and we dont want to do if any child is hoverd, then we cant know it,
//       since the hover for the outer box will be done first and it will hover and then the child and it will hover, 
//       there is no way to opt in our out of thi.
//       ---
//     This might be solved by having the default way of inputs, either the deepest child that has id or interactable 
//     or what we do right now, just by the rect. But then we would have to have some modifiers that would change the logic.
//     One way to do this would be to have a flag that makes the box hot if any immediate child is hot.
//     Another is if any inner child is hot. 

//     I am not sure where i need this yet and how example, so will just leave this here for now like this,
//     but i am writing this here for reasons to remind me of this all if i ever need a more specific logic
//     for hover and active.
// */
// UI_Actions ui_actions_from_box(UI_Box* this_frames_box)
// {
//   // TODO: You have to use a scissor rect here and then to recursive intersections if any parent in the tree for the box has clip flags set

//   UI_Actions* result_actions = &this_frames_box->actions;
//   if (this_frames_box->has_been_updated_this_build) { return *result_actions; }
  
//   this_frames_box->has_been_updated_this_build = true;
    
//   // We dont update a box that doesnt have id on it
//   if (this_frames_box->id.count == 0) { return *result_actions; } 
      
//   // We dont update boxes that are created this frame and were not present last frame
//   UI_Box* prev_frames_box = ui_get_box_prev_frame(this_frames_box->id);
//   if (ui_box_is_zero(prev_frames_box)) { Assert(IsZeroStruct(*result_actions)); *result_actions; } 

//   UI_State* ctx = ui_get_state();

//   // Data to get
//   B32 is_hovered                = false;
//   B32 is_down                   = false;
//   B32 was_down                  = false;
//   B32 left_box_while_was_down   = false;
//   B32 is_active                 = false;
//   B32 is_navigated              = false;

//   B32 some_other_box_is_being_interacted_with = (
//     ctx->interacted_with_box_id.count != 0 // There is a box that is interacted with right now
//     &&
//     !str8_match(ctx->interacted_with_box_id, prev_frames_box->id, 0) // We are not the box that is interacted with right now
//   );

//   RangeV2F32 interactable_bbox = intersect_range_v2f32(prev_frames_box->clip_bbox, prev_frames_box->final_on_screen_bbox);

//   is_hovered = rangeV2F32_within(interactable_bbox, ui_get_mouse_pos());

//   // Either there is no active box or we are the active box
//   // Since interacted box data is retained across frame boundary, 
//   // we just load the retained state and possibly update it here.
//   // No need to load hover, we get it each frame just from the box rect.
//   if (!some_other_box_is_being_interacted_with)
//   {
//     was_down                      = ctx->interacted_with_box_id__is_mouse_down;
//     left_box_while_was_down       = ctx->interacted_with_box_id__did_mouse_leave_box_while_was_down;

//     if (is_hovered && !was_down) // Mouse is up, check if we it goes down
//     {
//       // note: This has a bit of de sync relative to the is_hovered bool since we test if is hovered based on a different mouse pos than the one that was when the mouse went down, most of the time this shoud be fine, but i am not sure about the other times
//       //       Might be nice to use mouse_pos from the prev frame or somethign like that, for now it should be fine
//       B32 mouse_left_went_down = false;
//       {
//         OS_Event_list* events = os_get_frame_event_list();
//         for (OS_Event* ev = events->first; ev; ev = ev->next)
//         {
//           if (ev->kind == OS_Event_kind__mouse && ev->mouse_event.button == Mouse_button__left && ev->mouse_event.went_down)
//           {
//             mouse_left_went_down = true;
//             os_consume_frame_event(ev);
//           }
//         }
//       }

//       if (mouse_left_went_down)  
//       {
//         // New box is interacted, so setting the state for it
//         Assert(!was_down);
//         Assert(!left_box_while_was_down);
//         Assert(!ctx->interacted_with_box_id__is_mouse_down);
//         Assert(!ctx->interacted_with_box_id__did_mouse_leave_box_while_was_down);
//         Assert(str8_match(ctx->interacted_with_box_id, Str8{}, 0));

//         is_down = true;
//         ctx->interacted_with_box_id__is_mouse_down = true;
//         ctx->interacted_with_box_id__did_mouse_leave_box_while_was_down = false;
//         ctx->interacted_with_box_id = str8_copy(ui_get_build_arena(), this_frames_box->id);
//       }
//     }
//     else if (was_down) 
//     {
//       is_down = true;

//       if (!is_hovered && is_down) { 
//         left_box_while_was_down = true; 
//         ctx->interacted_with_box_id__did_mouse_leave_box_while_was_down = true;
//       }

//       // todo: The events api sucks right now, but if it works, i will make a better one
//       B32 mouse_left_went_up = false;
//       {
//         OS_Event_list* events = os_get_frame_event_list();
//         for (OS_Event* ev = events->first; ev; ev = ev->next)
//         {
//           if (ev->kind == OS_Event_kind__mouse && ev->mouse_event.button == Mouse_button__left && ev->mouse_event.went_up)
//           {
//             mouse_left_went_up = true;
//             os_consume_frame_event(ev);
//             break;
//           }
//         }
//       }

//       if (mouse_left_went_up)
//       {
//         Assert(was_down);
//         Assert(ctx->interacted_with_box_id__is_mouse_down);

//         is_down = false;
//         ctx->interacted_with_box_id__is_mouse_down                      = false;
//         ctx->interacted_with_box_id__did_mouse_leave_box_while_was_down = false;
//         ctx->interacted_with_box_id                                     = Str8{};
//       }
//     }
//   }

//   is_active    = str8_match(ctx->active_box_id, this_frames_box->id, 0);
//   is_navigated = str8_match(ctx->navigated_box_id, this_frames_box->id, 0);

//   result_actions->is_hovered              = is_hovered;            
//   result_actions->is_down                 = is_down;               
//   result_actions->was_down                = was_down;              
//   result_actions->left_box_while_was_down = left_box_while_was_down;
//   result_actions->is_clicked              = was_down && !is_down && !left_box_while_was_down;
//   result_actions->went_down               = !was_down && is_down;
//   result_actions->went_up                 = was_down && !is_down;  
//   result_actions->is_active               = is_active;
//   // result_actions->is_navigated            = is_navigated;

//   return *result_actions;
// }

// UI_Actions ui_actions_from_id(Str8 id)
// {
//   UI_Actions actions = {};
//   UI_Box* box = ui_get_box_prev_frame(id);
//   if (!ui_box_is_zero(box)) { actions = ui_actions_from_box(box); }
//   return actions;
// }

// ///////////////////////////////////////////////////////////
// // - Some new stuff that is yet unstructured
// //
// void ui_set_active_id(Str8 id)
// {
//   if (id.count == 0) { return; }

//   UI_State* ctx = ui_get_state();

//   // I guess this is how it is supposed to work, not sure about reset yet thought
//   ctx->active_box_id    = str8_copy(ui_get_build_arena(), id);
//   ctx->navigated_box_id = str8_copy(ui_get_build_arena(), id);
// }

// void ui_set_active_box(UI_Box* box)
// {
//   ui_set_active_id(box->id);
// }

// void ui_reset_active_id(Str8 id)
// {
//   UI_State* ctx = ui_get_state();
//   if (str8_match(ctx->active_box_id, id, 0)) {
//     ui_reset_active();
//   }
// }

// void ui_reset_active()
// {
//   UI_State* ctx = ui_get_state();
//   ctx->active_box_id = Str8{};
// }

// B32 ui_is_active_id(Str8 id)
// {
//   if (id.count == 0) { return false; }
//   UI_State* ctx = ui_get_state();
//   return str8_match(ctx->active_box_id, id, 0);
// }

// B32 ui_is_active_box(UI_Box* box)
// {
//   return ui_is_active_id(box->id);
// }

// B32 ui_has_active() 
// {
//   return (ui_get_state()->active_box_id.count != 0);
// }

// void ui_set_b_color(UI_Box* box, V4F32 color)
// {
//   if (ui_box_is_zero(box)) { return; }
//   box->vertex_colors[UV__x0y0] = color;
//   box->vertex_colors[UV__x0y1] = color;
//   box->vertex_colors[UV__x1y0] = color;
//   box->vertex_colors[UV__x1y1] = color;
// }

// void ui_set_cursor(OS_Cursor cursor) 
// {
//   UI_State* ctx = ui_get_state();
//   ctx->final_cursor = cursor;
// }

// ///////////////////////////////////////////////////////////
// // - Style stacks
// //

// // note: this is done via memcpy and not =, since in c/cpp = works like memcpy, but it does not work for arrays of fixes size, which i sometimes use, for example for color per vertex, mem cpy makes it work with static fixed size arrays and with values.
// #define _UI_StyleStackPush_Impl(ctx_p, stack_name_inside_ctx, node_type, val) \
//   node_type* node = ArenaPush(ctx_p->style_stacks_arena, node_type);          \
//   node->v = val;                                                              \
//   StackPush(&ctx_p->stack_name_inside_ctx, node);                             \
//   ctx_p->stack_name_inside_ctx.count += 1;

// #define _UI_StyleStackPop_Impl(ctx_p, stack_name_inside_ctx, node_type)  \
//   if (ctx_p->stack_name_inside_ctx.count > 0) {                          \
//     StackPop(&ctx_p->stack_name_inside_ctx);                             \
//     ctx_p->stack_name_inside_ctx.count -= 1;                             \
//     ctx_p->stack_name_inside_ctx.pop_after_first_use = false;            \
//   }

// #define _UI_StyleStackGet_Impl(ctx_p, stack_name_inside_ctx, node_type, name_for_default_value_var) \
//   if (ctx_p->stack_name_inside_ctx.first != 0) {                                                    \
//     return ctx_p->stack_name_inside_ctx.first->v;                                                   \
//   } else {                                                                                          \
//     return ctx_p->defaults.name_for_default_value_var;                                              \
//   }                                                                    

// #define _UI_StyleStackSetNext_Impl(ctx_p, stack_name_inside_ctx, node_type, val) \
//   if (ctx_p->stack_name_inside_ctx.pop_after_first_use) {                        \
//     _UI_StyleStackPop_Impl(ctx_p, stack_name_inside_ctx, node_type);             \
//   }                                                                              \
//   _UI_StyleStackPush_Impl(ctx_p, stack_name_inside_ctx, node_type, val);         \
//   ctx_p->stack_name_inside_ctx.pop_after_first_use = true;

// #define _U_StyleStackPopSigngleUsage_Imp(ctx_p, stack_name_inside_ctx, node_type) \
//   if (ctx->stack_name_inside_ctx.pop_after_first_use) {                           \
//     _UI_StyleStackPop_Impl(ctx_p, stack_name_inside_ctx, node_type)               \
//   }

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
  ui_push_border_left(border);
  ui_push_border_right(border);
  ui_push_border_top(border);
  ui_push_border_bottom(border);
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

Clay_BorderWidth __ui_clay_border_width_from_v4f32(V4F32 border)
{
  Clay_BorderWidth clay_border = {};
  clay_border.left            = (U16)border.v[0];
  clay_border.right           = (U16)border.v[1];
  clay_border.top             = (U16)border.v[2];
  clay_border.bottom          = (U16)border.v[3];
  clay_border.betweenChildren = {}; // Not sure if we need this, so not using this yet
  return clay_border;
}

#endif











