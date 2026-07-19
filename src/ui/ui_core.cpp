#ifndef __UI_CPP
#define __UI_CPP

#include "core/core_include.h"
#include "core/core_include.cpp"

#include "font_provider/font_provider.h"
#include "font_provider/font_provider.cpp"

#include "draw/draw.h"
#include "draw/draw.cpp"

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

UI_CUSTOM_DRAW_BOX_DEF(__ui_custom_draw_stub_func)
{
  BreakPoint(
    "Hey big fella."
    "I guess some went wrong since you are here,"
    "but dont worry, I believe in you."
  );
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
  Arena* state_arena = arena_alloc(Megabytes(8));
  __ui_g_state = ArenaPush(state_arena, UI_State);
  __ui_g_state->state_arena = state_arena;

  StaticAssert(ArrayCount(__ui_g_state->build_arenas) == 2);
  __ui_g_state->build_arenas[0] = arena_alloc(Megabytes(8));
  __ui_g_state->build_arenas[1] = arena_alloc(Megabytes(8));

  { // Clay arena 
    U64 mem_size_for_clay    = Clay_MinMemorySize();
    Arena* arena_for_clay    = arena_alloc(mem_size_for_clay);
    U8* bytes_for_clay_arena = ArenaPushArr(arena_for_clay, U8, mem_size_for_clay);
    Clay_Arena arena         = Clay_CreateArenaWithCapacityAndMemory(mem_size_for_clay, bytes_for_clay_arena);
    Clay_Initialize(arena, Clay_Dimensions{ 100, 100 }, Clay_ErrorHandler{ __ui_error_handler_for_clay, 0 });
    __ui_g_state->arena_for_clay = arena_for_clay;
  }

  __ui_g_state->current_build_root_box = ui_null_box();
  __ui_g_state->prev_build_root_box    = ui_null_box();

  __ui_g_state->first_free_box = ui_null_box();
}

void ui_release()
{
  StaticAssert(ArrayCount(__ui_g_state->build_arenas) == 2);
  arena_release(&__ui_g_state->build_arenas[0]);
  arena_release(&__ui_g_state->build_arenas[1]);

  arena_release(&__ui_g_state->arena_for_clay);
  arena_release(&__ui_g_state->state_arena);
  __ui_g_state = 0;
}

///////////////////////////////////////////////////////////
// - UI building
//
void ui_begin_build(V2F32 window_dims, V2F32 mouse_pos, FP_Font default_font)
{   
  ProfBeginFunc();
  UI_State* state = ui_get_state();
  
  state->build_generation += 1;

  { // DD: Making sure that null box has not been modified last frame by someone 
    B32 comp = {};
    UI_Box valid_null_box = __UI_NULL_BOX_VALUE;
    MemCompareSafe(__ui_g_null_box, valid_null_box, &comp);
    Assert(comp);
  }

  // DD: Cleaning the cashe hash table 
  for EachIndex(bucket_index, ArrayCount(state->hash_table_buckets))
  {
    UI_Box_list* bucket = state->hash_table_buckets + bucket_index;
    for (
      UI_Box* box = bucket->first, *next_box = ui_null_box(); 
      !ui_is_null_box(box); 
      box = next_box
    ) {
      next_box = box->next_in_bucket_or_free_list;
      
      // DD: If the box has not been "used" for a single build we remove it from the box cashe hash table
      if ((box->generation_when_last_used + 1) != state->build_generation)
      {
        // DD: Removing the box from the bucket list
        DllPop_Name(bucket, box, first, last, next_in_bucket_or_free_list, prev_in_bucket);
        bucket->count -= 1;

        // DD: Adding the box to the state free list
        box = ui_null_box();
        StackPush_Explicit_Ex(state->first_free_box, box, next_in_bucket_or_free_list, ui_is_null_box);
        state->count_of_free_boxes += 1;
      }
    }
  }

  // DD: This also removed all the boxes that were created for a single build since those are allocated on reused build arenas
  arena_clear(ui_get_build_arena());
  
  state->final_hover_box                       = ui_null_box();
  state->prev_build_root_box                   = state->current_build_root_box;
  state->current_build_root_box                = ui_null_box();
  state->render_commands_as_result_of_ui_build =  {};

  // DD: Resetting all the stacks
  #define UI_RESET_STACKS(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, set_next_func_name, pop_func_name, auto_pop_func_name, get_top_func_name, stack_arr_capacity, defer_push_pop_macro_name) \
    state->stacks.var_name_inside_state = {}; \
    state->stacks.var_name_inside_state.default_value = default_expr; 
  __UI_STACK_DATA_TABLE_EXPANSION(UI_RESET_STACKS)
  #undef UI_RESET_STACKS

  state->mouse_pos_for_prev_build   = state->mouse_pos_for_this_build;
  state->mouse_pos_for_this_build   = mouse_pos;
  state->window_dims_for_this_build = window_dims;

  ui_push_font(default_font);

  ui_next_width(ui_px(window_dims.x));
  ui_next_height(ui_px(window_dims.y));
  state->current_build_root_box = ui_box_make(Str8FromC("__UI_ROOT_BOX__"), UI_Box_flag__NONE);

  ui_push_parent(state->current_build_root_box);

  // DD:
  // Clay works in builds. You have to start a clay build by calling Clay_BeginLayout
  // to have access to the data that go produces last build. For example any 
  // Clay_ElementData or anything like that is from the prev buils, we
  // will be using it in our own build, to have access to the actual previous build
  // and not the one before last one, so we start Clay build here.
  Clay_SetPointerState({ state->mouse_pos_for_this_build.x, state->mouse_pos_for_this_build.y }, false);
  Clay_SetLayoutDimensions({ state->window_dims_for_this_build.x, state->window_dims_for_this_build.y });
  Clay_UpdateScrollContainers(false, {}, {}); 
  Clay_BeginLayout();

  ProfEndGroup();
}

void ui_end_build()
{
  ProfBeginFunc();

  ui_pop_parent();

  UI_State* state = ui_get_state();

  // Ending Clay build
  __ui_build_clay_element_tree_from_box_tree(state->current_build_root_box);
  Clay_RenderCommandArray clay_render_commands = Clay_EndLayout();
  state->render_commands_as_result_of_ui_build = clay_render_commands; 

  if (!ui_is_null_box(state->final_hover_box))
  {
    os_set_cursor(state->final_hover_box->per_build_data.hover_cursor);
  }

  ProfEndGroup();
}

// TODO: Look into this here again
void __ui_build_clay_element_tree_from_box_tree(UI_Box* root)
{
  if (ui_is_null_box(root)) { return; }
  UI_State* state = ui_get_state();

  Clay__OpenElement();
  Clay_ElementDeclaration clay_config = {};

  // DD: Clay__HashString returns a non 0 value when you give it a {} string and then you have collisions 
  if (root->per_build_data.id.count != 0) { Assert(root->hash_table_key.v != 0); }
  if (!ui_is_null_box_key(root->hash_table_key))
  {
    clay_config.id = Clay__HashString(__ui_clay_string_from_str8(root->per_build_data.id), 0, 0);
  }

  clay_config.layout.sizing.width  = __ui_clay_sizing_axis_from_ui_size(root->per_build_data.size_on_axis[Axis2__x]);
  clay_config.layout.sizing.height = __ui_clay_sizing_axis_from_ui_size(root->per_build_data.size_on_axis[Axis2__y]);
  clay_config.layout.layoutDirection = (root->per_build_data.layout_direction == Axis2__x ?  CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM);
  
  clay_config.layout.padding  = __ui_clay_padding_from_v4f32(root->per_build_data.padding);
  clay_config.layout.childGap = (U16)root->per_build_data.child_gap;
  
  if (0) {}
  else if (root->per_build_data.alignment_on_x == UI_Alignment_x__left)   { clay_config.layout.childAlignment.x = CLAY_ALIGN_X_LEFT; }
  else if (root->per_build_data.alignment_on_x == UI_Alignment_x__center) { clay_config.layout.childAlignment.x = CLAY_ALIGN_X_CENTER; }
  else if (root->per_build_data.alignment_on_x == UI_Alignment_x__right)  { clay_config.layout.childAlignment.x = CLAY_ALIGN_X_RIGHT; }

  if (0) {}
  else if (root->per_build_data.alignment_on_y == UI_Alignment_y__top)   { clay_config.layout.childAlignment.y = CLAY_ALIGN_Y_TOP; }
  else if (root->per_build_data.alignment_on_y == UI_Alignment_y__center) { clay_config.layout.childAlignment.y = CLAY_ALIGN_Y_CENTER; }
  else if (root->per_build_data.alignment_on_y == UI_Alignment_y__bottom)  { clay_config.layout.childAlignment.y = CLAY_ALIGN_Y_BOTTOM; }

  clay_config.backgroundColor = __ui_clay_color_from_v4f32(root->per_build_data.b_color); 
  clay_config.cornerRadius    = __ui_clay_corner_radius_from_v4f32(root->per_build_data.corner_radii); 

  clay_config.clip.horizontal  = root->per_build_data.clip_axis[Axis2__x];
  clay_config.clip.vertical    = root->per_build_data.clip_axis[Axis2__x];
  clay_config.clip.childOffset = { root->clip_offset.x, root->clip_offset.y };
  // TODO: What do we do about the offset, do we set it here or nah

  clay_config.border = { __ui_clay_color_from_v4f32(root->per_build_data.border_color), __ui_clay_border_width_from_v4f32(root->per_build_data.border_width) };
  
  // TODO: Deal with the fact that clay doesnt allow for single axis float, Assert for now
  if (root->per_build_data.flags & UI_Box_flag__floating)
  {
    clay_config.floating.pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_CAPTURE; // Damian: Not sure where i need this, so just const right now
    clay_config.floating.attachTo           = CLAY_ATTACH_TO_PARENT;             // Damian: Not sure where i need this, so just const right now
    clay_config.floating.clipTo             = CLAY_CLIP_TO_NONE;                 // Damian: Not sure where i need this, so just const right now
    clay_config.floating.offset.x = root->per_build_data.floating_fixed_pos.x; 
    clay_config.floating.offset.y = root->per_build_data.floating_fixed_pos.y; 
    // Clay_Dimensions expand;
    // uint32_t parentId;
    // int16_t zIndex;
    // Clay_FloatingAttachPoints attachPoints;
    // Clay_PointerCaptureMode pointerCaptureMode;
    // Clay_FloatingAttachToElement attachTo;
    // Clay_FloatingClipToElement clipTo;
  }

  // DD: We dont use these
  // config->aspectRatio = {};  
  // config->image       = {};  

  if ( root->per_build_data.custom_draw_extension.draw_func != 0 
    && root->per_build_data.custom_draw_extension.draw_func != __ui_custom_draw_stub_func
  ) {
    clay_config.custom.customData = root;
  }

  // DD: Setting user data to be the box itself so we then can go back from clay_element to UI_Box if needed
  clay_config.userData = root;

  Clay__ConfigureOpenElementPtr(&clay_config);

  if (Clay_Hovered())
  {
    if (root->per_build_data.has_hover_cursor)
    {
      state->final_hover_box = root;
    }
  }

  // DD: Traversing the tree
  for (
    UI_Box* child = root->per_build_data.first_child; 
    !ui_is_null_box(child); 
    child = child->per_build_data.next_sibling
  ) {
    __ui_build_clay_element_tree_from_box_tree(child);
  }

  Clay__CloseElement();
}

///////////////////////////////////////////////////////////
// - Box making
//
B32 ui_is_null_box(UI_Box* box)
{
  return (box == 0) || (box == &__ui_g_null_box);
}

UI_Box* ui_null_box()
{
  return &__ui_g_null_box;
}



// TODO: Redo this now that we have a table
UI_Box* ui_box_make(Str8 id, UI_Box_flags flags)
{
  UI_State* state = ui_get_state();
  
  UI_Box_key key = ui_box_key_from_str8(id);

  UI_Box* box                 = ui_box_from_key(key);
  B32 is_box_new              = ui_is_null_box(box);
  B32 is_box_for_single_build = ui_is_null_box_key(key);
  
  // DD: Allocating the box if new box 
  if (is_box_new)
  {
    if (state->count_of_free_boxes == 0) { Assert(ui_is_null_box(state->first_free_box));  }
    if (state->count_of_free_boxes != 0) { Assert(!ui_is_null_box(state->first_free_box)); }

    if (is_box_for_single_build)
    {
      box = ArenaPush(ui_get_build_arena(), UI_Box);
    }
    else 
    {
      box = state->first_free_box;
      if(!ui_is_null_box(box))
      {
        StackPop_Explicit_Ex(state->first_free_box, next_in_bucket_or_free_list, ui_is_null_box);
        state->count_of_free_boxes -= 1;
      }
      else
      { 
        box = ArenaPush(state->state_arena, UI_Box);
      }
    }
  }
  
  // DD: Setting up the box
  if (is_box_new) 
  {
    *box = __ui_g_null_box;
  }
  box->generation_when_created   = ui_get_build_generation();
  box->generation_when_last_used = ui_get_build_generation();
  box->hash_table_key            = key;
  
  // DD: Resetting the per build data
  box->per_build_data = {};
  
  // DD: Setting up the box
  { 
    box->per_build_data.id = str8_copy(ui_get_build_arena(), id);
    
    box->per_build_data.flags                  = flags | ui_top_extra_flags();
    box->per_build_data.size_on_axis[Axis2__x] = ui_top_size_x();
    box->per_build_data.size_on_axis[Axis2__y] = ui_top_size_y();
    box->per_build_data.layout_direction       = ui_top_layout();

    if (box->per_build_data.flags & UI_Box_flag__has_padding)   { box->per_build_data.padding = ui_top_padding(); }
    if (box->per_build_data.flags & UI_Box_flag__has_child_gap) { box->per_build_data.child_gap = ui_top_child_gap(); }

    box->per_build_data.alignment_on_x = ui_top_alignment_x();
    box->per_build_data.alignment_on_y = ui_top_alignment_y();

    if (box->per_build_data.flags & UI_Box_flag__has_background)      { box->per_build_data.b_color = ui_top_b_color(); }
    if (box->per_build_data.flags & UI_Box_flag__has_rounded_corners) { box->per_build_data.corner_radii = ui_top_corner_radius(); }

    if (box->per_build_data.flags & UI_Box_flag__clip_x) { box->per_build_data.clip_axis[Axis2__x] = true; }
    if (box->per_build_data.flags & UI_Box_flag__clip_y) { box->per_build_data.clip_axis[Axis2__y] = true; }

    if (box->per_build_data.flags & UI_Box_flag__has_borders) { 
      box->per_build_data.border_width = ui_top_border_width(); 
      box->per_build_data.border_color = ui_top_border_color(); 
    }

    // Damian: Right now we only have default behaviour on Floating
    box->per_build_data.floating_fixed_pos = v2f32(ui_top_floating_fixed_pos_x(), ui_auto_pop_floating_fixed_pos_y());    

    box->per_build_data.text_extension.font       = ui_top_font();
    box->per_build_data.text_extension.font_size  = ui_top_font_size();
    box->per_build_data.text_extension.font_color = ui_top_font_color();

    // Only having a non default cursor if there is one, disregard the default cursor
    if ( state->stacks.stack_hover_cursor.count > 0 
      || state->stacks.stack_hover_cursor.is_single_use_value_set
    )  {
      box->per_build_data.hover_cursor = ui_top_hover_cursor();
      if (box->per_build_data.hover_cursor != OS_Cursor__arrow) { box->per_build_data.has_hover_cursor = true; }
    }
  }

  // TODO: What the fuck is this here
  state->next_new_elements_parent_box = ui_top_parent();
  box->per_build_data.parent = state->next_new_elements_parent_box;
  if (!ui_is_null_box(box->per_build_data.parent))
  {
    DllPushBack_Name_NullFunc(box->per_build_data.parent, box, per_build_data.first_child, per_build_data.last_child, per_build_data.next_sibling, per_build_data.prev_sibling, ui_is_null_box);
    box->per_build_data.parent->per_build_data.children_count += 1;
  }

  // DD: Auto popping all the stacks
  #define __UI_AUTO_POP_ALL_THE_STACKS(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, set_next_func_name, pop_func_name, auto_pop_func_name, get_top_func_name, stack_arr_capacity, defer_push_pop_macro_name) \
    auto_pop_func_name();
  __UI_STACK_DATA_TABLE_EXPANSION(__UI_AUTO_POP_ALL_THE_STACKS)
  #undef __UI_AUTO_POP_ALL_THE_STACKS

  return box;
}

// Damian: Testing reverse param order 
UI_Box* ui_box_make_n(UI_Box_flags flags, Str8 id)
{
  return ui_box_make(id, flags);
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

/*
void __ui_get_next_box_clay_element_config(UI_Box* box, Clay_ElementId clay_id, UI_Box_flags flags)
{
  flags |= ui_top_extra_flags();
  box->flags = flags;

  Clay_ElementDeclaration* config = &box->clay_element_config;

  config->id = clay_id;

  config->layout.sizing.width    = __ui_clay_sizing_axis_from_ui_size(ui_top_size_x());
  config->layout.sizing.height   = __ui_clay_sizing_axis_from_ui_size(ui_top_size_y());
  config->layout.layoutDirection = (ui_top_layout() == Axis2__x ?  CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM);
  
  if (flags & UI_Box_flag__has_padding) { config->layout.padding = __ui_clay_padding_from_v4f32(ui_top_padding()); }
  if (flags & UI_Box_flag__has_child_gap) { config->layout.childGap = (U16)ui_top_child_gap(); }
  
  // TODO: If this gets used than thave this be a helper that goes from your type to clay type and in reverse
  {
    UI_Alignment_x al = ui_top_alignment_x();
    if (0) {}
    else if (al == UI_Alignment_x__left) { config->layout.childAlignment.x = CLAY_ALIGN_X_LEFT; }
    else if (al == UI_Alignment_x__center) { config->layout.childAlignment.x = CLAY_ALIGN_X_CENTER; }
    else if (al == UI_Alignment_x__right) { config->layout.childAlignment.x = CLAY_ALIGN_X_RIGHT; }
  }

  // TODO: If this gets used than thave this be a helper that goes from your type to clay type and in reverse
  {
    UI_Alignment_y al = ui_top_alignment_y();
    if (0) {}
    else if (al == UI_Alignment_y__top) { config->layout.childAlignment.y = CLAY_ALIGN_Y_TOP; }
    else if (al == UI_Alignment_y__center) { config->layout.childAlignment.y = CLAY_ALIGN_Y_CENTER; }
    else if (al == UI_Alignment_y__bottom) { config->layout.childAlignment.y = CLAY_ALIGN_Y_BOTTOM; }
  }

  if (flags & UI_Box_flag__has_background) { config->backgroundColor = __ui_clay_color_from_v4f32(ui_top_background_color()); }
  if (flags & UI_Box_flag__has_rounded_corners) { config->cornerRadius = __ui_clay_corner_radius_from_v2f32(ui_top_corner_radius()); }

  if (flags & UI_Box_flag__clip_x) { config->clip.horizontal = true; }
  if (flags & UI_Box_flag__clip_y) { config->clip.vertical = true; }

  if (flags & UI_Box_flag__has_borders) { config->border = { __ui_clay_color_from_v4f32(ui_top_border_color()), __ui_clay_border_width_from_v4f32(ui_top_border_width()) }; }
  
  // TODO: Deal with the fact that clay doesnt allow for single axis float, Assert for now
  if (!(flags & UI_Box_flag__floating) && ((flags & UI_Box_flag__floating_x) || (flags & UI_Box_flag__floating_y))) { InvalidCodePath(); }
  if (flags & UI_Box_flag__floating)
  {
    config->floating.pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_CAPTURE; // Damian: Not sure where i need this, so just const right now
    config->floating.attachTo           = CLAY_ATTACH_TO_PARENT;             // Damian: Not sure where i need this, so just const right now
    config->floating.clipTo             = CLAY_CLIP_TO_NONE;                 // Damian: Not sure where i need this, so just const right now
    // Clay_Vector2 offset;
    // Clay_Dimensions expand;
    // uint32_t parentId;
    // int16_t zIndex;
    // Clay_FloatingAttachPoints attachPoints;
    // Clay_PointerCaptureMode pointerCaptureMode;
    // Clay_FloatingAttachToElement attachTo;
    // Clay_FloatingClipToElement clipTo;
  }

  config->aspectRatio = {}; // TODO: 
  config->image       = {}; // TODO: 
}
*/

// TODO: This is not used right now
Str8 __ui_get_id_part_from_str8(Str8 str)
{
  RangeU64 range_for_double_hash = str8_find(str, Str8FromC("##"), 0);
  RangeU64 range_for_triple_hash = str8_find(str, Str8FromC("###"), 0);

  Str8 id_part = str;
  if (rangeU64_count(range_for_triple_hash) > 0)
  {
    id_part = str8_substring(str, range_for_triple_hash.min, range_for_triple_hash.max);
  }

  return id_part;
}

// TODO: This is not used right now
Str8 __ui_get_text_part_from_str8(Str8 str)
{
  RangeU64 range_for_double_hash = str8_find(str, Str8FromC("##"), 0);
  RangeU64 range_for_triple_hash = str8_find(str, Str8FromC("###"), 0);

  Str8 text_part = str;
  if (rangeU64_count(range_for_triple_hash) > 0)
  {
    text_part = str8_substring(str, 0, range_for_triple_hash.min);
  }
  else if (rangeU64_count(range_for_double_hash) > 0)
  {
    text_part = str8_substring(str, 0, range_for_double_hash.min);
  }

  return text_part;
}

///////////////////////////////////////////////////////////
// - Box extension
//
void ui_extend_box_with_custom_draw_function(UI_Box* box, UI_Box_custom_draw_func_pointer_type* custom_draw, void* data) // TODO: Need a better name when you are sure what this does and is
{
  InvalidCodePath();
  // box->custom_draw_extension.draw_func          = custom_draw;
  // box->custom_draw_extension.data_for_draw_func = (void*)data;
}

void ui_extend_box_with_text(UI_Box* box, Str8 str)
{
  InvalidCodePath();
  // box->text_extension.text = str8_copy(ui_get_build_arena(), str);

  // Damian: These are already in the `text_extension`. 
  // box->text_extension.font_size = ui_top_font_size();
  // box->text_extension.font      = ui_top_font();
}

///////////////////////////////////////////////////////////
// - Box data
//
// UI_Box_data ui_box_data_from_id(Str8 id)
// {
//   UI_Box* prev_build_box = ui_find_prev_build_box_by_id(id);
//   UI_Box_data data = ui_box_data_from_box(prev_build_box);
//   return data;
// }

/*
UI_Box_data ui_box_data_from_box(UI_Box* box)
{
  if (ui_is_null_box(box)) { return {}; }
  
  UI_Box_data box_data = {};
  if (box->generation_when_created < box->generation_when_last_used)
  {
    box_data.is_found = true;
    box_data.rect = box->rect;
    InvalidCodePath();
    // TODO: Here you padd up the prev rect or some like that
  }

  return box_data;

  // TODO: This should just have the thing at this point 
  // and so you just have to return it via box->rect

  // {
  //   Clay_ElementId clay_id             = __ui_clay_element_id_from_str8(box->id);
  //   Clay_ElementData clay_element_data = Clay_GetElementData(clay_id);

  //   box_data.is_found   = true;
  //   box_data.rect       = ;
  //   box_data.inner_rect = ;


  // }

  // Assert(clay_element_data.found); // DD: Since we have UI_Box* that we got from prev build we also should have the box in clay then, if not this is a bug
  // if (clay_element_data.found)
  // {
  //   V4F32 paddings = {};
  //   if (prev_build_box->flags & UI_Box_flag__has_padding) { paddings = prev_build_box->padding; }
    
  //   result_data.is_found   = true;
  //   result_data.rect       = __ui_rect_from_clay_bounding_box(clay_element_data.boundingBox);
  //   result_data.inner_rect = rect_padded_ex(result_data.rect, v4f32_scale(paddings, -1.0f));
  // }

  // return result_data;
}
*/

///////////////////////////////////////////////////////////
// - Box clip data
//
/*
UI_Box_clip_data ui_box_clip_data_from_box(UI_Box* box)
{
  if (ui_is_null_box(box)) { return {}; }
  UI_Box_clip_data data = ui_box_clip_data_from_id(box->id);
  return data;
}

UI_Box_clip_data ui_box_clip_data_from_id(Str8 id)
{
  if (id.count == 0) { return {}; }

  UI_Box_clip_data result_clip_data = {};

  Clay_ElementId clay_id = __ui_clay_element_id_from_str8(id);
  Clay_ScrollContainerData clay_scroll_data = Clay_GetScrollContainerData(clay_id);
  if (clay_scroll_data.found)
  {
    result_clip_data.is_found      = true;
    result_clip_data.viewport_dims = __ui_v2f32_from_clay_dimensions(clay_scroll_data.scrollContainerDimensions);
    result_clip_data.content_dims  = __ui_v2f32_from_clay_dimensions(clay_scroll_data.contentDimensions);
  }

  return result_clip_data;
}
*/

///////////////////////////////////////////////////////////
// - Box actions
//
UI_Actions ui_actions_from_box(UI_Box* box)
{
  if (ui_is_null_box(box))                     { return {}; }
  if (box->actions_present)                    { return box->actions; }
  if (ui_is_null_box_key(box->hash_table_key)) 
  { 
    BreakPoint("You probably ment ther to be an id to this box thought"); 
    box->actions_present = true; 
    return {}; 
  }

  UI_State* state = ui_get_state();

  // Data to get
  B32 is_hovered                 = false;
  B32 is_down                    = false;
  B32 was_down                   = false;
  B32 left_box_while_was_down    = false;
  B32 is_active                  = false;
  B32 is_navigated               = false;
  V2F32 mouse_pos_when_went_down = {};

  is_hovered = Clay_PointerOver(__ui_clay_element_id_from_str8(box->per_build_data.id)); // TODO: See if this gets the most nested box or just checked if the mouse is inside the box's rect

  B32 some_other_box_is_being_interacted_with = (
    !ui_is_null_box_key(state->interacted_with_box_data.box_key) 
    &&
    !ui_box_key_match(state->interacted_with_box_data.box_key, box->hash_table_key)
  );

  // Damian:
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
        Assert(IsZeroStruct(state->interacted_with_box_data.pos_when_mouse_went_down));
        Assert(ui_is_null_box_key(state->interacted_with_box_data.box_key));

        is_down = true;
        mouse_pos_when_went_down = ui_get_mouse_pos();
        state->interacted_with_box_data.is_mouse_down                      = true;
        state->interacted_with_box_data.did_mouse_leave_box_while_was_down = false;
        state->interacted_with_box_data.box_key                            = box->hash_table_key;
        state->interacted_with_box_data.pos_when_mouse_went_down           = ui_get_mouse_pos();
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
        state->interacted_with_box_data.pos_when_mouse_went_down           = v2f32(0, 0);
        state->interacted_with_box_data.box_key                            = ui_null_box_key();
      }
    }
  }

  // is_active = str8_match(ctx->active_box_id, this_frames_box->id, 0);

  UI_Actions result_actions = {};
  result_actions.is_hovered               = is_hovered;            
  result_actions.is_down                  = is_down;               
  result_actions.was_down                 = was_down;              
  result_actions.left_box_while_was_down  = left_box_while_was_down;
  result_actions.is_clicked               = was_down && !is_down && !left_box_while_was_down;
  result_actions.went_down                = !was_down && is_down;
  result_actions.went_up                  = was_down && !is_down;  
  result_actions.mouse_pos_when_went_down = mouse_pos_when_went_down;

  box->actions_present = true;
  box->actions = result_actions;

  return result_actions;
}

///////////////////////////////////////////////////////////
// - Box clip offset
//
/*
V2F32 ui_clip_offset_from_box(UI_Box* box)
{
  return box->clip_offset;
}

V2F32 ui_clip_offset_from_id(Str8 id)
{
  // TODO: I dont like this here, this might get fixed if we dont have 2 builds at the same time, but 
  //       rather just reuse boxes from the prev build that have to stay and then just 
  //       resue the data in then that we need to be cross frame present
  V2F32 offset = {};
  UI_Box* this_buid_box = ui_find_box_in_tree_by_id(ui_get_root(), id);
  if (!ui_is_null_box(this_buid_box)) 
  {
    offset = this_buid_box->clip_offset;
  }
  else 
  {
    UI_Box* prev_buid_box = ui_find_prev_build_box_by_id(id);
    if (!ui_is_null_box(prev_buid_box))
    {
    offset = prev_buid_box->clip_offset;
    }
  }
  return offset;
}

// TODO: Have this not be this way
void ui_set_clip_offset_from_box(UI_Box* box, V2F32 offset)
{
  box->clip_offset = offset;
}
void ui_set_clip_offset_from_id(Str8 id, V2F32 offset)
{
  UI_Box* box = ui_find_box_in_tree_by_id(ui_get_root(), id);
  if (!ui_is_null_box(box))
  {
    box->clip_offset = offset;
  }
}

///////
// TODO: These are to be moved from here or removed from the codebase
V2F32 ui_get_prev_build_scroll_for_box(UI_Box* box)
{
  V2F32 prev_offset = {};
  UI_Box* prev_build_box = ui_find_prev_build_box_by_box(box);
  if (!ui_is_null_box(prev_build_box))
  {
    prev_offset = prev_build_box->clip_offset;
  }
  return prev_offset;
}
*/

///////////////////////////////////////////////////////////
// - Box setters
//
/*
void ui_box_set_clip_offset_for_axis(UI_Box* box, F32 clip_offset, Axis2 axis)
{
  box->clip_offset.v[axis] = clip_offset;
}

void ui_box_set_clip_offset_x(UI_Box* box, F32 clip_offset)
{
  ui_box_set_clip_offset_for_axis(box, clip_offset, Axis2__x);
}

void ui_box_set_clip_offset_y(UI_Box* box, F32 clip_offset)
{
  ui_box_set_clip_offset_for_axis(box, clip_offset, Axis2__y);
}

void ui_box_set_clip_offset(UI_Box* box, V2F32 clip_offset)
{
  ui_box_set_clip_offset_x(box, clip_offset.x);
  ui_box_set_clip_offset_y(box, clip_offset.y);
}

void ui_id_set_clip_offset_for_axis(Str8 id, F32 clip_offset, Axis2 axis) 
{
  UI_Box* this_buids_root = ui_get_root();
  UI_Box* box = ui_find_box_in_tree_by_id(this_buids_root, id);
  ui_box_set_clip_offset_for_axis(box, clip_offset, axis);
}

void ui_id_set_clip_offset_x(Str8 id, F32 clip_offset) 
{
  UI_Box* this_buids_root = ui_get_root();
  UI_Box* box = ui_find_box_in_tree_by_id(this_buids_root, id);
  ui_box_set_clip_offset_x(box, clip_offset);
}

void ui_id_set_clip_offset_y(Str8 id, F32 clip_offset) 
{
  UI_Box* this_buids_root = ui_get_root();
  UI_Box* box = ui_find_box_in_tree_by_id(this_buids_root, id);
  ui_box_set_clip_offset_y(box, clip_offset);
}

void ui_id_set_clip_offset(Str8 id, V2F32 clip_offset) 
{
  UI_Box* this_buids_root = ui_get_root();
  UI_Box* box = ui_find_box_in_tree_by_id(this_buids_root, id);
  ui_box_set_clip_offset(box, clip_offset);
}
*/

///////////////////////////////////////////////////////////
// - Box key stuff
//

UI_Box_key ui_null_box_key()
{
  UI_Box_key key = {};
  return key;
}

B32 ui_box_key_match(UI_Box_key key, UI_Box_key other)
{
  return (key.v == other.v);
}

B32 ui_is_null_box_key(UI_Box_key key)
{
  return ui_box_key_match(key, ui_null_box_key());
}

UI_Box_key ui_box_key_from_str8(Str8 str)
{
  UI_Box_key key = {};
  for EachIndex(i, str.count) // TODO: Need a better way to make a key
  {
    key.v += str.data[i];
  }
  return key;
}

UI_Box* ui_box_from_key(UI_Box_key key)
{
  ProfBeginFunc();
  if (ui_is_null_box_key(key)) { return ui_null_box(); }
  
  UI_State* state     = ui_get_state();
  U64 bucket_index    = key.v % ArrayCount(state->hash_table_buckets); 
  UI_Box_list* bucket = state->hash_table_buckets + bucket_index;
  
  UI_Box* result_box = ui_null_box();
  for (UI_Box* box = bucket->first; !ui_is_null_box(box); box = box->next_in_bucket_or_free_list)
  {
    if (ui_box_key_match(key, box->hash_table_key))
    {
      result_box = box;
      break;
    }
  }

  ProfEndGroup();
  return result_box;
}

///////////////////////////////////////////////////////////
// - UI drawing
//
void ui_draw()
{
  // TODO: Find a way to reliably have scissor rect pushing and popping for boxes
  //       that have no_overdraw flag set

  UI_State* state = ui_get_state();
  Clay_RenderCommandArray render_commands = state->render_commands_as_result_of_ui_build;

  d_push_scissor_rect(rect_make(0.0f, 0.0f, state->window_dims_for_this_build.x, state->window_dims_for_this_build.y));

  B32 got_green_border = false;

  for EachIndex(command_index, render_commands.length)
  {
    // DD: 
    // There is a weird case in clay where there is shared data inside render command
    // type, you would expect then to always be set to valid data. Docs for .userData
    // say this "A pointer transparently passed through from the original element declaration.".
    // This would mean that .userData is set all the time to what i set it to. So if i 
    // set it to X then it should always be X when i get it from the renderCommand type.
    // But this is not the case. For CLAY_RENDER_COMMAND_TYPE_SCISSOR_START and
    // CLAY_RENDER_COMMAND_TYPE_SCISSOR_END they are set to 0. Which to me seems missleading.

    Clay_RenderCommand command = render_commands.internalArray[command_index];
    Rect rect = __ui_rect_from_clay_bounding_box(command.boundingBox);
    
    switch (command.commandType)
    {
      case CLAY_RENDER_COMMAND_TYPE_NONE:
      default: { } break;

      case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
      {
        V4F32 color        = __ui_v4f32_from_clay_color(command.renderData.rectangle.backgroundColor);
        V4F32 corner_radii = __ui_v4f32_from_clay_corner_radius(command.renderData.rectangle.cornerRadius);

        F32 softness = 0.0f; // Keeping softness as a var thought used only once for later search when we get to having softness used in rendering
        d_draw_rect_pro(rect, color, color, color, color, corner_radii, softness);
      } break;

      case CLAY_RENDER_COMMAND_TYPE_BORDER:
      {
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
        InvalidCodePath();

        /*
        // Damian: Just making sure
        Assert((UI_Box*)command.renderData.custom.customData == (UI_Box*)command.userData);
        
        V4F32 b_color              = __ui_v4f32_from_clay_color(command.renderData.custom.backgroundColor);
        V4F32 clay_corner_r        = __ui_v4f32_from_clay_corner_radius(command.renderData.custom.cornerRadius);
        UI_Box* box_to_custom_draw = (UI_Box*)command.renderData.custom.customData;

        Assert(box_to_custom_draw->custom_draw_extension.draw_func);
        if (box_to_custom_draw->custom_draw_extension.draw_func)
        {
          UI_Provided_data_for_custom_draw provided_data = {};
          provided_data.box              = box_to_custom_draw;
          provided_data.final_box_rect   = rect;
          provided_data.background_color = b_color;
          provided_data.corner_radii     = clay_corner_r;
          
          box_to_custom_draw->custom_draw_extension.draw_func(provided_data);
        }

        */
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
  if (kind == UI_Size_kind__percent_of_parent)
  {
    Assert(value1 == value2);
    clamp_f32_inplace(&value1, 0.0f, 1.0f);
    clamp_f32_inplace(&value2, 0.0f, 1.0f);
  }

  UI_Size size = {};
  size.kind   = kind;
  size.value1 = value1;
  size.value2 = value2; 
  return size;
}
UI_Size ui_px(F32 value)             { return ui_size_make(UI_Size_kind__px, value, 0.0f); }
UI_Size ui_rem(F32 scale)            { return ui_px(ui_top_font_size() * scale); }                 
UI_Size ui_fit_mm(F32 min, F32 max)  { return ui_size_make(UI_Size_kind__fit, min, max); } 
UI_Size ui_grow_mm(F32 min, F32 max) { return ui_size_make(UI_Size_kind__grow, min, max); }         
UI_Size ui_fit()                     { return ui_size_make(UI_Size_kind__fit, 0.0f, 0.0f); } 
UI_Size ui_grow()                    { return ui_size_make(UI_Size_kind__grow, 0.0f, 0.0f); }         
UI_Size ui_p_of_p(F32 p)             { return ui_size_make(UI_Size_kind__percent_of_parent, p, p); }         

///////////////////////////////////////////////////////////
// - Other
//
U64 ui_get_build_generation()
{
  return ui_get_state()->build_generation;
}

Arena* ui_get_build_arena()
{
  U64 index = ui_get_state()->build_generation % 2;
  Arena* arena = ui_get_state()->build_arenas[index];
  return arena;
}

V2F32 ui_get_mouse_pos()
{
  return ui_get_state()->mouse_pos_for_this_build;
}

V2F32 ui_get_prev_mouse_pos()
{
  return ui_get_state()->mouse_pos_for_prev_build;
}

UI_Box* ui_get_current_parent()
{
  return ui_get_state()->next_new_elements_parent_box;
}

UI_Box* ui_get_root()
{
  return ui_get_state()->current_build_root_box;
}

// ================
// === OLD CODE ===


/*
UI_Box* ui_find_box_in_tree_by_id(UI_Box* root, Str8 id)
{
  if (id.count == 0)               { return ui_null_box(); }
  if (ui_is_null_box(root))        { return ui_null_box(); }
  if (str8_match(root->id, id, 0)) { return root; }

  UI_Box* found_box = ui_null_box();
  for (UI_Box* child = root->first_child; !ui_is_null_box(child); child = child->next_sibling)
  {
    found_box = ui_find_box_in_tree_by_id(child, id);
    if (!ui_is_null_box(found_box))
    {
      break;
    }
  }
  return found_box;
}

UI_Box* ui_find_prev_build_box_by_id(Str8 id)
{
  if (id.count == 0) { return ui_null_box(); }
  UI_Box* found_box = ui_find_box_in_tree_by_id(ui_get_state()->prev_build_root_box, id);
  return found_box;
}

UI_Box* ui_find_prev_build_box_by_box(UI_Box* box)
{
  UI_Box* prev_frame_box = ui_find_prev_build_box_by_id(box->id);
  return prev_frame_box;
}
*/

///////////////////////////////////////////////////////////
// - Stack functions and helper
//
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DEFINE_PUSH_FUNC)
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DEFINE_SET_NEXT_FUNC)
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DEFINE_POP_FUNC)
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DEFINE_AUTO_POP_FUNC)
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DEFINE_TOP_FUNC)

///////////////////////////////////////////////////////////
// - Stack function helpers (padding)
//
V4F32 ui_top_padding()
{
  V4F32 padding = {};
  padding.v[RectEdge__left]   = ui_top_padding_left();
  padding.v[RectEdge__right]  = ui_top_padding_right();
  padding.v[RectEdge__top]    = ui_top_padding_top();
  padding.v[RectEdge__bottom] = ui_top_padding_bottom();
  return padding;
}

void ui_next_padding(F32 padding)
{
  ui_next_padding_left(padding);
  ui_next_padding_top(padding);
  ui_next_padding_right(padding);
  ui_next_padding_bottom(padding);
}

void ui_push_padding(F32 padding)
{
  ui_push_padding_left(padding);
  ui_push_padding_right(padding);
  ui_push_padding_top(padding);
  ui_push_padding_bottom(padding);
}

void ui_pop_padding()
{
  ui_pop_padding_left();
  ui_pop_padding_right();
  ui_pop_padding_top();
  ui_pop_padding_bottom();
}

void ui_next_padding_ex(F32 left, F32 right, F32 top, F32 down)
{
  ui_next_padding_left(left);
  ui_next_padding_right(right);
  ui_next_padding_top(top);
  ui_next_padding_bottom(down);
}

///////////////////////////////////////////////////////////
// - Stack function helpers (sizing)
//
void ui_next_width(UI_Size size)  { ui_next_size_x(size); }
void ui_next_height(UI_Size size) { ui_next_size_y(size); }
void ui_next_size_axis(Axis2 axis, UI_Size size)
{
  if (0) {}
  else if (axis == Axis2__x) { ui_next_size_x(size); }
  else if (axis == Axis2__y) { ui_next_size_y(size); }
}

///////////////////////////////////////////////////////////
// - Stack function helpers (background color)
//
V4F32 ui_top_b_color()            { return ui_top_background_color(); }
void ui_next_b_color(V4F32 color) { ui_next_background_color(color); }
void ui_push_b_color(V4F32 color) { ui_push_background_color(color); }
void ui_pop_b_color()             { ui_pop_background_color(); }

///////////////////////////////////////////////////////////
// - Stack function helpers (corner radius)
//
V4F32 ui_top_corner_radius()
{
  V4F32 corner_r = {};
  corner_r.v[UV__top_left]     = ui_top_corner_radius_top_left();
  corner_r.v[UV__top_right]    = ui_top_corner_radius_top_right();
  corner_r.v[UV__bottom_left]  = ui_top_corner_radius_bottom_left();
  corner_r.v[UV__bottom_right] = ui_top_corner_radius_bottom_right();
  return corner_r;
}

void ui_next_corner_r(F32 r)
{
  ui_next_corner_radius_top_left(r);
  ui_next_corner_radius_top_right(r);
  ui_next_corner_radius_bottom_right(r);
  ui_next_corner_radius_bottom_left(r);
}

void ui_push_corner_r(F32 r)
{
  ui_push_corner_radius_top_left(r);
  ui_push_corner_radius_top_right(r);
  ui_push_corner_radius_bottom_right(r);
  ui_push_corner_radius_bottom_left(r);
}

void ui_pop_corner_r()
{
  ui_pop_corner_radius_top_left();
  ui_pop_corner_radius_top_right();
  ui_pop_corner_radius_bottom_right();
  ui_pop_corner_radius_bottom_left();
}

///////////////////////////////////////////////////////////
// - Stack function helpers (border width)
//
V4F32 ui_top_border_width()
{
  V4F32 border = {};
  border.v[RectEdge__left]   = ui_top_border_left();
  border.v[RectEdge__right]  = ui_top_border_right();
  border.v[RectEdge__top]    = ui_top_border_top();
  border.v[RectEdge__bottom] = ui_top_border_bottom();
  return border;
}

void ui_next_border_width(F32 border)
{
  ui_next_border_left(border);
  ui_next_border_right(border);
  ui_next_border_top(border);
  ui_next_border_bottom(border);
}

void ui_push_border_width(F32 border)
{
  ui_push_border_left(border);
  ui_push_border_right(border);
  ui_push_border_top(border);
  ui_push_border_bottom(border);
}

void ui_pop_border_width()
{
  ui_pop_border_left();
  ui_pop_border_right();
  ui_pop_border_top();
  ui_pop_border_bottom();
}

///////////////////////////////////////////////////////////
// - Stack function helpers (border)
//
void ui_next_border(F32 width, V4F32 color)
{
  ui_next_border_width(width);
  ui_next_border_color(color);
}

void ui_push_border(F32 width, V4F32 color)
{
  ui_push_border_width(width);
  ui_push_border_color(color);
}

void ui_pop_border()
{
  ui_pop_border_width();
  ui_pop_border_color();
}

///////////////////////////////////////////////////////////
// - Stack function helpers (padded border)
//
void ui_next_padded_border(F32 width, V4F32 color)
{
  ui_next_border(width, color);
  ui_next_padding(width);
}

void ui_push_padded_border(F32 width, V4F32 color)
{
  ui_push_border(width, color);
  ui_push_padding(width);
}

void ui_pop_padded_border()
{
  ui_pop_border();
  ui_pop_padding();
}

///////////////////////////////////////////////////////////
// - Stack function helpers (layout)
//
void ui_next_layout_x() { ui_next_layout(Axis2__x); }
void ui_next_layout_y() { ui_next_layout(Axis2__y); }

///////////////////////////////////////////////////////////
// - Box style setters for already created boxed
//
void ui_box_set_b_color(UI_Box* box, V4F32 color)
{
  box->per_build_data.b_color = color;
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

V4F32 __ui_v4f32_from_clay_padding(Clay_Padding clay_padding)
{
  V4F32 padding = {};
  padding.v[RectEdge__left]   = (F32)clay_padding.left;
  padding.v[RectEdge__right]  = (F32)clay_padding.right;
  padding.v[RectEdge__top]    = (F32)clay_padding.top;
  padding.v[RectEdge__bottom] = (F32)clay_padding.bottom;
  return padding;
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

Clay_CornerRadius __ui_clay_corner_radius_from_v4f32(V4F32 vec)
{
  Clay_CornerRadius clay_crs = {};
  clay_crs.topLeft     = vec.v[UV__top_left];
  clay_crs.topRight    = vec.v[UV__top_right];
  clay_crs.bottomLeft  = vec.v[UV__bottom_left];
  clay_crs.bottomRight = vec.v[UV__bottom_right];
  return clay_crs;
}

Clay_ElementId __ui_clay_element_id_from_str8(Str8 str)
{
  Clay_String clay_str = __ui_clay_string_from_str8(str);
  Clay_ElementId clay_id = Clay__HashString(clay_str, 0, 0);
  return clay_id;
}

V2F32 __ui_v2f32_from_clay_dimensions(Clay_Dimensions clay_dims)
{
  V2F32 dims = {};
  dims.x = clay_dims.width;
  dims.y = clay_dims.height;
  return dims;
}

Clay_Dimensions __ui_clay_dimensions_from_v2f32(V2F32 vec)
{
  Clay_Dimensions clay_dims = {};
  clay_dims.width  = vec.x;
  clay_dims.height = vec.y;
  return clay_dims;
}

///////////////////////////////////////////////////////////
// - Move this to a better place
//
void __ui_error_handler_for_clay(Clay_ErrorData errorText)
{
  BreakPoint();
}

#endif











