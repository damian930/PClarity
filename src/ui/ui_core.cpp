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

///////////////////////////////////////////////////////////
// - State variables
//
UI_State* __ui_g_state = 0;
UI_Box __ui_g_null_box = {};

///////////////////////////////////////////////////////////
// - State accessors
//
UI_State* ui_get_state() { return __ui_g_state; }
void ui_set_state(UI_State* state) { __ui_g_state = state; }

///////////////////////////////////////////////////////////
// - State 
//
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

  __ui_g_state->current_build_root_box = ui_box_null();

  __ui_g_state->first_free_box = ui_box_null();

  __ui_box_set_to_null_mem(&__ui_g_null_box);
}

void ui_release()
{
  StaticAssert(ArrayCount(__ui_g_state->build_arenas) == 2);
  arena_release(&__ui_g_state->build_arenas[0]);
  arena_release(&__ui_g_state->build_arenas[1]);

  arena_release(&__ui_g_state->arena_for_clay);
  arena_release(&__ui_g_state->state_arena);
  __ui_g_state = 0;

  // DD: Clay gets release by itself
  Clay_SetCurrentContext(0);
}

///////////////////////////////////////////////////////////
// - UI building
//
void ui_begin_build(V2F32 window_dims, V2F32 mouse_pos, FP_Font default_font)
{   
  ProfBeginFunc();
  UI_State* state = ui_get_state();
  
  state->build_generation += 1;

  // DD: Making sure that null box has not been modified last frame by someone 
  { 
    B32 comp = false;
    UI_Box test_null_box = {}; 
    __ui_box_set_to_null_mem(&test_null_box);
    MemCompareSafe(__ui_g_null_box, test_null_box, &comp);
    Assert(comp); 
  }

  state->last_build_box_count = state->this_build_box_count;
  state->this_build_box_count = 0;

  state->final_context_menu_key_for_prev_build = state->open_context_menu_box_key;

  // DD: Cleaning the cashe hash table 
  for EachIndex(bucket_index, ArrayCount(state->hash_table_buckets))
  {
    UI_Box_list* bucket = state->hash_table_buckets + bucket_index;
    for (
      UI_Box* box = bucket->first, *next_box = ui_box_null(); 
      !ui_box_is_null(box); 
      box = next_box
    ) {
      next_box = box->next_in_bucket_or_free_list;

      // DD: If the box has not been "used" for a single build we remove it from the box cashe hash table
      if ((box->generation_when_last_created + 1) != state->build_generation)
      {
        // DD: Removing the box from the bucket list
        DllPop_Ex(bucket, box, first, last, next_in_bucket_or_free_list, prev_in_bucket, ui_box_is_null, ui_box_null());
        bucket->count -= 1;

        // DD: Nulling the box and adding the box to the state free list
        __ui_box_set_to_null_mem(box);
        StackPush_Explicit_Ex(state->first_free_box, box, next_in_bucket_or_free_list, ui_box_is_null, ui_box_null());
        state->count_of_free_boxes += 1;
      }
    }
  }

  // DD: This also removed all the boxes that were created for a single build since those are allocated on reused build arenas
  arena_clear(ui_get_build_arena());
  
  state->final_hover_box                       = ui_box_null();
  state->current_build_root_box                = ui_box_null();
  state->render_commands_as_result_of_ui_build = Clay_RenderCommandArray{};
  
  // DD: Resetting all the stacks
  #define UI_RESET_STACKS(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, set_next_func_name, pop_func_name, auto_pop_func_name, get_top_func_name, stack_arr_capacity) \
          state->stacks.var_name_inside_state = {}; \
          state->stacks.var_name_inside_state.default_value = default_expr; 
  __UI_STACK_DATA_TABLE_EXPANSION(UI_RESET_STACKS)
  #undef UI_RESET_STACKS

  state->mouse_pos_for_prev_build   = state->mouse_pos_for_this_build;
  state->mouse_pos_for_this_build   = mouse_pos;
  state->window_dims_for_this_build = window_dims;

  Assert(!state->remove_context_menu_when_closing_it);
  state->remove_context_menu_when_closing_it = false;

  // DD: Handling the closing of the context menu if pressed outside of it 
  if (!ui_box_key_is_null(state->final_context_menu_key_for_prev_build))
  {
    UI_Box* prev_build_conetext_menu_box = ui_box_from_key(state->final_context_menu_key_for_prev_build);

    if (!rect_point_inside(prev_build_conetext_menu_box->rect, mouse_pos))
    {
      for (OS_Event* ev = os_get_frame_event_list()->first; ev; ev = ev->next)
      {
        if (ev->kind == OS_Event_kind__mouse && ev->mouse_event.went_down)
        {
          state->open_context_menu_box_key = ui_box_key_null();
          state->open_context_menu_offset  = {};
          break;
        }
      }
    } 
  }

  ui_push_font(default_font);

  // DD: Making of the root box for the ui
  ui_next_width(ui_px(window_dims.x));
  ui_next_height(ui_px(window_dims.y));
  state->current_build_root_box = ui_box_make(UI_Box_flag__NONE, Str8FromC("__UI_ROOT_BOX__"));

  ui_push_parent(state->current_build_root_box);

  // DD:
  // Clay works in builds. You have to start a clay build by calling Clay_BeginLayout
  // to have access to the data that go produces last build. For example any 
  // Clay_ElementData or anything like that is from the prev buils, we
  // will be using it in our own build, to have access to the actual previous build
  // and not the one before last one, so we start Clay build here.
  // ==
  // Right now(28th of July 2026) we only use Clay_PointerOver inside ui_actions_from_box
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

  // TODO: This is new stuff
  {
    if (!ui_box_key_is_null(state->open_context_menu_box_key))
    {
      UI_Box* context_menu_box = ui_box_from_key(state->open_context_menu_box_key);
      Assert(!ui_box_is_null(context_menu_box));
      if (!ui_box_is_null(context_menu_box))
      {
        if (context_menu_box->generation_when_last_created != ui_get_build_generation())
        {
          state->open_context_menu_box_key = ui_box_key_null();
          state->open_context_menu_offset  = V2F32{};
        }
      }
    }

    // TODO: Inside the context menu box into the root box if context menu box is present
    if (!ui_box_key_is_null(state->open_context_menu_box_key))
    {
      UI_Box* root = ui_get_root();
      UI_Box* context_menu_box = ui_box_from_key(state->open_context_menu_box_key);
      
      Assert(!ui_box_is_null(context_menu_box));
      if (!ui_box_is_null(context_menu_box))
      {
        context_menu_box->per_build_config.parent = root;
        DllPushBack_Explicit_Ex(root->per_build_config.first_child, root->per_build_config.last_child, context_menu_box, per_build_config.next_sibling, per_build_config.prev_sibling, ui_box_is_null, ui_box_null());
        root->per_build_config.children_count += 1;
      }
    }
  }


  // DD: Making clay boxes from our own box tree
  __ui_build_clay_element_tree_from_box_tree(state->current_build_root_box);

  Clay_RenderCommandArray clay_render_commands = Clay_EndLayout();
  
  __ui_store_persistant_data_for_persistant_boxes_after_clay_done_laying_out(state->current_build_root_box);
  
  state->render_commands_as_result_of_ui_build = clay_render_commands; 

  if (!ui_box_is_null(state->final_hover_box))
  {
    os_set_cursor(state->final_hover_box->per_build_config.hover_cursor);
  }

  ProfEndGroup();
}

///////////////////////////////////////////////////////////
// - Box making
//
UI_Box* ui_box_make(UI_Box_flags flags, Str8 id)
{
  UI_State* state = ui_get_state();
  
  state->this_build_box_count += 1;

  UI_Box_key new_box_key      = ui_box_key_from_str8(id); 
  UI_Box* box                 = ui_box_from_key(new_box_key);
  B32 is_box_new              = ui_box_is_null(box);
  B32 is_box_for_single_build = ui_box_key_is_null(new_box_key);
  
  if (box->generation_when_last_created == state->build_generation)
  {
    BreakPoint("You got a duplicate id buddy");
  }

  // DD: Allocating the box if new box 
  if (is_box_new)
  {
    if (state->count_of_free_boxes == 0) { Assert(ui_box_is_null(state->first_free_box));  }
    if (state->count_of_free_boxes != 0) { Assert(!ui_box_is_null(state->first_free_box)); }

    // DD: Allocating the box
    if (is_box_for_single_build)
    {
      box = ArenaPush(ui_get_build_arena(), UI_Box);
    }
    else 
    {
      box = state->first_free_box;
      if(!ui_box_is_null(box))
      {
        StackPop_Explicit_Ex(state->first_free_box, next_in_bucket_or_free_list, ui_box_is_null);
        state->count_of_free_boxes -= 1;
      }
      else
      { 
        box = ArenaPush(state->state_arena, UI_Box);
      }
    }

    // DD: Setting up shared state for single build boxes and persistant boxes
    __ui_box_set_to_null_mem(box);
    box->generation_when_first_created = ui_get_build_generation();

    // DD: Setting up state for persistant build boxes and adding them to the box hash table
    if (!is_box_for_single_build)
    {
      box->hash_table_key = new_box_key;
      U64 bucket_index    = box->hash_table_key.v % 64;
      UI_Box_list* bucket = state->hash_table_buckets + bucket_index;
      DllPushBack_Ex(bucket, box, first, last, next_in_bucket_or_free_list, prev_in_bucket, ui_box_is_null, ui_box_null());
      bucket->count += 1;
    }
  }
  if (!is_box_for_single_build) { Assert(ui_box_key_match(box->hash_table_key, new_box_key)); }
  
  box->generation_when_last_created = ui_get_build_generation();
  
  box->data_from_previous_build.flags   = box->per_build_config.flags;
  box->data_from_previous_build.padding = box->per_build_config.padding;

  if (box->defered_clip_offset.is_present)
  {
    box->clip_offset = box->defered_clip_offset.offset;
    box->defered_clip_offset = {};
  }

  // DD: Reallocating drag memory to the new build arena to not lose it
  box->dynamic_drag_memory = str8_copy(ui_get_build_arena(), box->dynamic_drag_memory);

  // DD: Resetting the per build data
  box->per_build_config = __ui_g_null_box.per_build_config;
  
  // DD: Putting the box in the build ui box tree
  {
    UI_Box** parent = &box->per_build_config.parent;
    *parent = ui_top_parent();
    if (!ui_box_is_null(*parent))
    {
      DllPushBack_Explicit_Ex((*parent)->per_build_config.first_child, (*parent)->per_build_config.last_child, box, per_build_config.next_sibling, per_build_config.prev_sibling, ui_box_is_null, ui_box_null());
      (*parent)->per_build_config.children_count += 1;
    }
  }

  // DD, TODO: This is new stuff, work on this more 
  box->prev_build_parent_context_menu_key = box->parent_context_menu_key;
  box->parent_context_menu_key = ui_box_key_null(); //state->open_context_menu_box_key;
  for (UI_Box* parent = box->per_build_config.parent; !ui_box_is_null(parent); parent = parent->per_build_config.parent)
  {
    if (ui_box_key_match(state->open_context_menu_box_key, parent->hash_table_key))
    {
      box->parent_context_menu_key = state->open_context_menu_box_key;
      break;
    }
  }

  // DD: Setting up the box config
  { 
    box->per_build_config.str_for_key = str8_copy(ui_get_build_arena(), id);
    
    box->per_build_config.flags                  = flags | ui_top_extra_flags();
    box->per_build_config.size_on_axis[Axis2__x] = ui_top_size_x();
    box->per_build_config.size_on_axis[Axis2__y] = ui_top_size_y();
    box->per_build_config.layout_direction       = ui_top_layout();

    if (box->per_build_config.flags & UI_Box_flag__has_padding)   { box->per_build_config.padding = ui_top_padding(); }
    if (box->per_build_config.flags & UI_Box_flag__has_child_gap) { box->per_build_config.child_gap = ui_top_child_gap(); }

    box->per_build_config.alignment_on_x = ui_top_alignment_x();
    box->per_build_config.alignment_on_y = ui_top_alignment_y();

    if (box->per_build_config.flags & UI_Box_flag__has_background) { box->per_build_config.b_color = ui_top_b_color(); }

    if (box->per_build_config.flags & UI_Box_flag__has_rounded_corners) { box->per_build_config.corner_radii = ui_top_corner_radius(); }

    if (box->per_build_config.flags & UI_Box_flag__clip_x) { box->per_build_config.clip_axis[Axis2__x] = true; }
    if (box->per_build_config.flags & UI_Box_flag__clip_y) { box->per_build_config.clip_axis[Axis2__y] = true; }

    box->per_build_config.outer_softness = ui_top_outer_softness();
    box->per_build_config.inner_softness = ui_top_inner_softness();

    if (box->per_build_config.flags & UI_Box_flag__has_borders) { 
      box->per_build_config.border_width = ui_top_border_width(); 
      box->per_build_config.border_color = ui_top_border_color(); 
    }

    // DD: Right now we only have default behaviour on Floating
    box->per_build_config.floating_fixed_pos = v2f32(ui_top_floating_fixed_pos_x(), ui_auto_pop_floating_fixed_pos_y());    
    if (
      state->stacks.stack_floating_fixed_dims_x.count > 0 || state->stacks.stack_floating_fixed_dims_x.is_single_use_value_set ||
      state->stacks.stack_floating_fixed_dims_y.count > 0 || state->stacks.stack_floating_fixed_dims_y.is_single_use_value_set
    ) {
      box->per_build_config.has_fixed_dims = true;
      box->per_build_config.floating_fixed_dims = v2f32(ui_top_floating_fixed_dims_x(), ui_auto_pop_floating_fixed_dims_y());    
    }

    box->per_build_config.floating_attach_point = ui_top_floating_attach_point();

    box->per_build_config.text_extension.font       = ui_top_font();
    box->per_build_config.text_extension.font_size  = ui_top_font_size();
    box->per_build_config.text_extension.font_color = ui_top_font_color();

    // Only having a non default cursor if there is one, disregard the default cursor
    if ( state->stacks.stack_hover_cursor.count > 0 
      || state->stacks.stack_hover_cursor.is_single_use_value_set
    )  {
      box->per_build_config.hover_cursor = ui_top_hover_cursor();
      if (box->per_build_config.hover_cursor != OS_Cursor__arrow) { box->per_build_config.has_hover_cursor = true; }
    }

    // DD: To not draw the overflow we have to scissor the final rect of the box.
    // To get the rect from a box via clay we have to call Clay_GetElementData.
    // Clay only gives it to us for boxes that have ids. 
    // So, if we need the rect for the box that has UI_Box_flag__dont_draw_overflow,
    // we have to make sure that we have it always. For this reason we generate an
    // id for a box if it doesnt have it already
    {
      if (box->per_build_config.flags & UI_Box_flag__dont_draw_overflow)
      {
        if (ui_box_key_is_null(box->hash_table_key))
        {
          box->per_build_config.str_for_key         = str8_fmt(ui_get_build_arena(), "__FAKE_ID_FOR_SINGLE_FRAME_%p__", (void*)box);
          box->per_build_config.is_str_for_key_fake = true;
        }
      }
    }

    // DD: Storing a link to the first parent that has no overdraw flag in the subtree above current
    {
      UI_Box* parent = box->per_build_config.parent;
      if (!ui_box_is_null(parent))
      {
        if ( parent->per_build_config.flags & UI_Box_flag__dont_draw_overflow 
          || !ui_box_is_null(parent->per_build_config.ancestor_with_no_overflow_drag_flag)
        ) { 
          box->per_build_config.ancestor_with_no_overflow_drag_flag = parent;
        }
      }
    }

  }

  // DD: Resetting actions
  box->actions_present = false;
  __ui_actions_set_to_null_mem(&box->actions);

  // DD: Auto popping all the stacks
  #define __UI_AUTO_POP_ALL_THE_STACKS(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, set_next_func_name, pop_func_name, auto_pop_func_name, get_top_func_name, stack_arr_capacity) \
    auto_pop_func_name();
  __UI_STACK_DATA_TABLE_EXPANSION(__UI_AUTO_POP_ALL_THE_STACKS)
  #undef __UI_AUTO_POP_ALL_THE_STACKS

  return box;
}

UI_Box* ui_box_make_f(UI_Box_flags flags, const char* fmt, ...)
{
  Scratch scratch = get_scratch(0, 0);
  va_list args;
  va_start(args, fmt);
  Str8 str = str8_valist(scratch.arena, fmt, args);
  UI_Box* box = ui_box_make(flags, str);
  va_end(args);
  end_scratch(&scratch);
  return box;
}

///////////////////////////////////////////////////////////
// - Box extension
//
void ui_extend_box_with_custom_draw_function(UI_Box* box, UI_Box_custom_draw_func* custom_draw, void* data) 
{
  box->per_build_config.custom_draw_extension.draw_func          = custom_draw;
  box->per_build_config.custom_draw_extension.data_for_draw_func = (void*)data;

  // DD: Giving this box a fake str id but not a key to then have clay give me the final rect 
  // to then be able to use in in the custom draw function
  if (box->per_build_config.str_for_key.count == 0)
  {
    box->per_build_config.str_for_key = str8_fmt(ui_get_build_arena(), "__FAKE_ID_FOR_CUSTOM_DATA_%p__", box);
  }
}

void ui_extend_box_with_text(UI_Box* box, Str8 str)
{
  box->per_build_config.text_extension.text = str8_copy(ui_get_build_arena(), str);

  // DD: These are already in the `text_extension`. 
  // box->text_extension.font_size = ui_top_font_size();
  // box->text_extension.font      = ui_top_font();
}

///////////////////////////////////////////////////////////
// - Box data
//
UI_Box_data ui_box_data_from_box(UI_Box* box)
{
  if (ui_box_is_null(box))                     { return {}; }
  if (ui_box_key_is_null(box->hash_table_key)) { return {}; }

  UI_Box_data box_data = {};
  if (box->generation_when_first_created != box->generation_when_last_created)
  {
    box_data.is_found   = true;
    box_data.rect       = box->rect;
    box_data.inner_rect = box->rect;
    if (box->per_build_config.flags & UI_Box_flag__has_padding) 
    {
      V4F32 padding = v4f32_scale(box->per_build_config.padding, -1.0f);
      box_data.inner_rect = rect_padded_ex(box_data.inner_rect, padding);
    }
  }

  return box_data;
}

UI_Box_data ui_box_data_from_id(Str8 id)
{
  UI_Box_key key   = ui_box_key_from_str8(id);
  UI_Box* box      = ui_box_from_key(key);
  UI_Box_data data = ui_box_data_from_box(box);
  return data;
}

///////////////////////////////////////////////////////////
// - Box clip data
//
UI_Box_clip_data ui_box_clip_data_from_box(UI_Box* box)
{
  if (ui_box_is_null(box))                     { return {}; }
  if (ui_box_key_is_null(box->hash_table_key)) { return {}; }

  UI_Box_clip_data box_data = {};
  if (box->generation_when_first_created != box->generation_when_last_created)
  {
    box_data.is_found      = true;
    box_data.viewport_dims = box->viewport_dims;
    box_data.content_dims  = box->content_dims;
    box_data.offset        = box->clip_offset;
  }

  return box_data;
}

UI_Box_clip_data ui_box_clip_data_from_id(Str8 id)
{
  UI_Box_key key        = ui_box_key_from_str8(id);
  UI_Box* box           = ui_box_from_key(key);
  UI_Box_clip_data data = ui_box_clip_data_from_box(box);
  return data;
}

///////////////////////////////////////////////////////////
// - Box actions
//
UI_Actions ui_actions_from_box(UI_Box* box)
{
  // TODO: Work on this more, there are some things that you dont understand about this right now, fix those

  // TODO: ui_actions_from_box should not be using any data from per_build_data
  //       since if we get this via id then we will have data for that from prev frame,
  //       but then when we get that via box* we dont have that, since the new boxes
  //       reset the per_build_data struct. This is a bug, redo this.

  if (ui_box_is_null(box))                     { return ui_box_null()->actions; }
  if (box->actions_present)                    { return box->actions; }
  if (ui_box_key_is_null(box->hash_table_key)) 
  { 
    BreakPoint("You probably ment ther to be an id to this box thought"); 
    box->actions_present = true; 
    return ui_box_null()->actions; 
  }
  if (box->generation_when_first_created == box->generation_when_last_created) // DD: Box just got made this build
  {
    box->actions_present = true;
    return ui_box_null()->actions;
  }
  
  UI_State* state = ui_get_state();

  B32 is_context_menu_open     = !ui_box_key_is_null(state->final_context_menu_key_for_prev_build);
  B32 is_child_of_context_menu = ui_box_key_match(box->prev_build_parent_context_menu_key, state->final_context_menu_key_for_prev_build);

  B32 do_inputs_for_this_box = true;
  if (is_context_menu_open && !is_child_of_context_menu)
  {
    do_inputs_for_this_box = false;
  }

  // DD: Data to get
  B32 is_hovered                                       = {};                                       
  B32 is_down                 [UI_Mouse_button__COUNT] = {}; 
  B32 was_down                [UI_Mouse_button__COUNT] = {}; 
  B32 left_box_while_was_down [UI_Mouse_button__COUNT] = {}; 
  B32 is_clicked              [UI_Mouse_button__COUNT] = {}; 
  B32 went_down               [UI_Mouse_button__COUNT] = {}; 
  B32 went_up                 [UI_Mouse_button__COUNT] = {}; 
  V2F32 mouse_pos_when_went_down                       = {};

  if (do_inputs_for_this_box)
  {
    is_hovered = Clay_PointerOver(__ui_clay_element_id_from_str8(box->per_build_config.str_for_key)); // TODO: See if this gets the most nested box or just checked if the mouse is inside the box's rect

    for EachEnumRange(button, UI_Mouse_button, UI_Mouse_button__left, UI_Mouse_button__COUNT)
    {
      B32 proceed_with_this_button = false;
      if (button == UI_Mouse_button__left && (box->per_build_config.flags & UI_Box_flag__left_clickable)) { proceed_with_this_button = true; }
      if (button == UI_Mouse_button__right && (box->per_build_config.flags & UI_Box_flag__right_clickable)) { proceed_with_this_button = true; }

      if (!proceed_with_this_button) { continue; }

      // UI_Box_key* box_key                            = &state->interacted_with_box_data[button].box_key;
      // B32*        is_mouse_down                      = &state->interacted_with_box_data[button].is_mouse_down;
      // B32*        did_mouse_leave_box_while_was_down = &state->interacted_with_box_data[button].did_mouse_leave_box_while_was_down;
      // V2F32*      pos_when_mouse_went_down           = &state->interacted_with_box_data[button].pos_when_mouse_went_down;

      B32 some_other_box_is_being_interacted_with = (
        !ui_box_key_is_null(state->interacted_with_box_data[button].box_key) 
        &&
        !ui_box_key_match(state->interacted_with_box_data[button].box_key, box->hash_table_key)
      );

      // DD:
      // Either there is no active box or we are the active box
      // Since interacted box data is retained across frame boundary, 
      // we just load the retained state and possibly update it here.
      // No need to load hover, we get it each frame just from the box rect.
      if (
        ((box->per_build_config.flags & UI_Box_flag__left_clickable) || (box->per_build_config.flags & UI_Box_flag__right_clickable))
        &&
        !some_other_box_is_being_interacted_with
      ) {
        was_down[button]                = state->interacted_with_box_data[button].is_mouse_down;
        left_box_while_was_down[button] = state->interacted_with_box_data[button].did_mouse_leave_box_while_was_down;
      
        // DD: Mouse is up, check if it goes down
        if (is_hovered && !was_down[button]) 
        {
          B32 mouse_left_went_down = false;
          {
            OS_Event_list* events = os_get_frame_event_list();
            for (OS_Event* ev = events->first; ev; ev = ev->next)
            {
              Mouse_button event_mouse_button = Mouse_button__left;
              if (button == UI_Mouse_button__right) { event_mouse_button = Mouse_button__right; }

              if (ev->kind == OS_Event_kind__mouse && ev->mouse_event.button == event_mouse_button && ev->mouse_event.went_down)
              {
                mouse_left_went_down = true;
                os_consume_frame_event(ev);
              }
            }
          }
    
          if (mouse_left_went_down)  
          {
            // We have a new interacted with box
            Assert(!was_down[button]);
            Assert(!left_box_while_was_down[button]);
            Assert(!state->interacted_with_box_data[button].is_mouse_down);
            Assert(!state->interacted_with_box_data[button].did_mouse_leave_box_while_was_down);
            Assert(IsZeroStruct(state->interacted_with_box_data[button].pos_when_mouse_went_down));
            Assert(ui_box_key_is_null(state->interacted_with_box_data[button].box_key));
    
            is_down[button] = true;
            mouse_pos_when_went_down = ui_get_mouse_pos();
            state->interacted_with_box_data[button].is_mouse_down                      = true;
            state->interacted_with_box_data[button].did_mouse_leave_box_while_was_down = false;
            state->interacted_with_box_data[button].box_key                            = box->hash_table_key;
            state->interacted_with_box_data[button].pos_when_mouse_went_down           = ui_get_mouse_pos();
          }
        }
        else if (was_down[button]) 
        {
          is_down[button] = true;
    
          if (!is_hovered && is_down[button]) { 
            left_box_while_was_down[button] = true; 
            state->interacted_with_box_data[button].did_mouse_leave_box_while_was_down = true;
          }
    
          // todo: The events api sucks right now, but it works, i will make a better one at some point ))
          B32 mouse_left_went_up = false;
          {
            Mouse_button event_mouse_button = Mouse_button__left;
            if (button == UI_Mouse_button__right) { event_mouse_button = Mouse_button__right; }

            OS_Event_list* events = os_get_frame_event_list();
            for (OS_Event* ev = events->first; ev; ev = ev->next)
            {
              if (ev->kind == OS_Event_kind__mouse && ev->mouse_event.button == event_mouse_button && ev->mouse_event.went_up)
              {
                mouse_left_went_up = true;
                os_consume_frame_event(ev);
                break;
              }
            }
          }
    
          if (mouse_left_went_up)
          {
            Assert(was_down[button]);
            Assert(state->interacted_with_box_data[button].is_mouse_down);
    
            is_down[button] = false;
            state->interacted_with_box_data[button].is_mouse_down                      = false;
            state->interacted_with_box_data[button].did_mouse_leave_box_while_was_down = false;
            state->interacted_with_box_data[button].pos_when_mouse_went_down           = v2f32(0, 0);
            state->interacted_with_box_data[button].box_key                            = ui_box_key_null();
          }
        }
      }

    }
  }

  UI_Actions result_actions = {};
  result_actions.is_hovered = is_hovered;

  result_actions.is_left_down                  = is_down[UI_Mouse_button__left];
  result_actions.was_left_down                 = was_down[UI_Mouse_button__left];
  result_actions.left_left_box_while_was_down  = left_box_while_was_down[UI_Mouse_button__left];
  result_actions.is_left_clicked               = was_down[UI_Mouse_button__left] && !is_down[UI_Mouse_button__left] && !left_box_while_was_down[UI_Mouse_button__left];
  result_actions.left_went_down                = !was_down[UI_Mouse_button__left] && is_down[UI_Mouse_button__left];
  result_actions.left_went_up                  = was_down[UI_Mouse_button__left] && !is_down[UI_Mouse_button__left];

  result_actions.is_right_down                 = is_down[UI_Mouse_button__right];
  result_actions.was_right_down                = was_down[UI_Mouse_button__right];
  result_actions.right_left_box_while_was_down = left_box_while_was_down[UI_Mouse_button__right];
  result_actions.is_right_clicked              = was_down[UI_Mouse_button__right] && !is_down[UI_Mouse_button__right] && !left_box_while_was_down[UI_Mouse_button__right];
  result_actions.right_went_down               = !was_down[UI_Mouse_button__right] && is_down[UI_Mouse_button__right];
  result_actions.right_went_up                 = was_down[UI_Mouse_button__right] && !is_down[UI_Mouse_button__right];
  
  result_actions.is_down                 = result_actions.is_left_down;
  result_actions.was_down                = result_actions.was_left_down;
  result_actions.left_box_while_was_down = result_actions.left_left_box_while_was_down;

  result_actions.is_clicked = result_actions.is_left_clicked;
  result_actions.went_down  = result_actions.left_went_down;
  result_actions.went_up    = result_actions.left_went_up;

  result_actions.mouse_pos_when_went_down = mouse_pos_when_went_down;
  result_actions.box                      = box;

  box->actions_present = true;
  box->actions = result_actions;

  return result_actions;
}

UI_Actions ui_actions_from_id(Str8 id)
{
  UI_Box_key key     = ui_box_key_from_str8(id);
  UI_Box* box        = ui_box_from_key(key);
  UI_Actions actions = ui_actions_from_box(box);
  return actions;
}

///////////////////////////////////////////////////////////
// - Box clip offset
//
V2F32 ui_box_clip_offset(UI_Box* box)
{
  V2F32 offset = {};
  if (!ui_box_is_null(box))
  {
    offset = box->clip_offset;
  }
  return offset;
}

V2F32 ui_box_clip_offset_by_id(Str8 id)
{
  UI_Box_key key = ui_box_key_from_str8(id);
  UI_Box* box    = ui_box_from_key(key);
  V2F32 offset   = ui_box_clip_offset(box);
  return offset;
}

///////////////////////////////////////////////////////////
// - Box key stuff
//
UI_Box_key ui_box_key_null()
{
  UI_Box_key key = {};
  return key;
}

B32 ui_box_key_match(UI_Box_key key, UI_Box_key other)
{
  return (key.v == other.v);
}

B32 ui_box_key_is_null(UI_Box_key key)
{
  return ui_box_key_match(key, ui_box_key_null());
}

UI_Box_key ui_box_key_from_str8(Str8 str)
{
  // DD: This is a bit changed version of "CLAY_DLL_EXPORT Clay_ElementId Clay__HashString(Clay_String key, uint32_t offset, uint32_t seed);"
  if (str.count == 0) { return ui_box_key_null(); }

  U64 hash = 0;
  
  U64 seed = 69;
  U64 base = seed;

  for (U64 i = 0; i < str.count; i++) 
  {
    base += str.data[i];
    base += (base << 10);
    base ^= (base >> 6);
  }
  hash = base;
  // hash += offset;
  hash += (hash << 10);
  hash ^= (hash >> 6);

  hash += (hash << 3);
  base += (base << 3);
  hash ^= (hash >> 11);
  base ^= (base >> 11);
  hash += (hash << 15);
  base += (base << 15);
  
  UI_Box_key box_key = {};
  box_key.v = hash;   
  return box_key;
}

UI_Box* ui_box_from_key(UI_Box_key key)
{
  // ProfBeginFunc();
  if (ui_box_key_is_null(key)) { return ui_box_null(); }
  
  UI_State* state     = ui_get_state();
  U64 bucket_index    = key.v % ArrayCount(state->hash_table_buckets); 
  UI_Box_list* bucket = state->hash_table_buckets + bucket_index;
  
  UI_Box* result_box = ui_box_null();
  for (UI_Box* box = bucket->first; !ui_box_is_null(box); box = box->next_in_bucket_or_free_list)
  {
    if (ui_box_key_match(key, box->hash_table_key))
    {
      result_box = box;
      break;
    }
  }

  // ProfEndGroup();
  return result_box;
}

///////////////////////////////////////////////////////////
// - Box drag memory
//
Data_buffer* ui_box_drag_buffer(UI_Box* box)
{
  return &box->dynamic_drag_memory;
}

Data_buffer* ui_box_drag_buffer_by_id(Str8 id)
{
  UI_Box_key key      = ui_box_key_from_str8(id);
  UI_Box* box         = ui_box_from_key(key);
  Data_buffer* buffer = ui_box_drag_buffer(box);
  return buffer;
}

Data_buffer* ui_box_drag_buffer_alloc(UI_Box* box, U64 size_to_alloc)
{
  box->dynamic_drag_memory = data_buffer_make(ui_get_build_arena(), size_to_alloc);
  return &box->dynamic_drag_memory;
}

Data_buffer* ui_box_drag_buffer_alloc_by_id(Str8 id, U64 size_to_alloc)
{
  UI_Box_key key      = ui_box_key_from_str8(id);
  UI_Box* box         = ui_box_from_key(key);
  Data_buffer* buffer = ui_box_drag_buffer_alloc(box, size_to_alloc);
  return buffer;
}

void ui_box_drag_buffer_release(UI_Box* box)
{
  box->dynamic_drag_memory = Data_buffer{};
}

void ui_box_drag_buffer_release_by_id(Str8 id)
{
  UI_Box_key key = ui_box_key_from_str8(id);
  UI_Box* box    = ui_box_from_key(key);
  ui_box_drag_buffer_release(box);
}

///////////////////////////////////////////////////////////
// - Box setters (clip offset) 
//
void ui_box_set_b_color(UI_Box* box, V4F32 color)
{
  box->per_build_config.b_color = color;
}

void ui_box_set_border_width(UI_Box* box, V4F32 border_width)
{
  box->per_build_config.border_width = border_width;
}

void ui_box_set_border_color(UI_Box* box, V4F32 border_color)
{
  box->per_build_config.border_color = border_color;
}

void ui_box_set_border(UI_Box* box, V4F32 border_width, V4F32 border_color)
{
  ui_box_set_border_width(box, border_width);
  ui_box_set_border_color(box, border_color);
}

void ui_box_set_clip_offset_for_axis(UI_Box* box, F32 clip_offset, Axis2 axis)
{
  box->clip_offset.v[axis] = clip_offset;
}

void ui_box_set_clip_offset_for_axis_by_id(Str8 id, F32 clip_offset, Axis2 axis)
{
  UI_Box_key key = ui_box_key_from_str8(id);
  UI_Box* box = ui_box_from_key(key);
  ui_box_set_clip_offset_for_axis(box, clip_offset, axis);
}

void ui_box_set_clip_offset_y(UI_Box* box, F32 offset)
{
  ui_box_set_clip_offset_for_axis(box, offset, Axis2__y);
}

void ui_box_set_clip_offset_x(UI_Box* box, F32 offset)
{
  ui_box_set_clip_offset_for_axis(box, offset, Axis2__x);
}

///////////////////////////////////////////////////////////
// - Null box
//
B32 ui_box_is_null(UI_Box* box)
{
  return (box == 0) || (box == &__ui_g_null_box);
}

UI_Box* ui_box_null()
{
  return &__ui_g_null_box;
}

///////////////////////////////////////////////////////////
// - UI drawing
//
void ui_draw()
{
  UI_State* state = ui_get_state();
  Clay_RenderCommandArray render_commands = state->render_commands_as_result_of_ui_build;

  d_push_scissor_rect(rect_make(0.0f, 0.0f, state->window_dims_for_this_build.x, state->window_dims_for_this_build.y));

  for EachIndex(command_index, render_commands.length)
  {
    // DD: 
    // There is a weird case in clay where there is shared data inside render command
    // type, you would expect them to always be set to valid data. Docs for .userData
    // say this "A pointer transparently passed through from the original element declaration.".
    // This would mean that .userData is set all the time to what i set it to. So if i 
    // set it to X then it should always be X when i get it from the renderCommand type.
    // But this is not the case. For CLAY_RENDER_COMMAND_TYPE_SCISSOR_START and
    // CLAY_RENDER_COMMAND_TYPE_SCISSOR_END they are set to 0. Which to me seems missleading.

    Clay_RenderCommand command = render_commands.internalArray[command_index];
    Rect rect = __ui_rect_from_clay_bounding_box(command.boundingBox);
    
    B32 pop_scissor_rect_after_this_command = false;
    if (command.userData != 0)
    { 
      UI_Box* box = (UI_Box*)command.userData;
      UI_Box* no_overdraw_parent = box->per_build_config.ancestor_with_no_overflow_drag_flag;
      if (!ui_box_is_null(no_overdraw_parent))
      {
        Rect scissor_rect = no_overdraw_parent->rect; // DD: Its okay to just use the rect here since this draw is called after ui_end_build which produces the final rect for boxes 
        if (d_get_state()->current_scissor_rect_count > 0)
        {
          Rect current_scissor_rect = __d_get_current_scissor_rect__defaults();
          scissor_rect = rect_intersect(no_overdraw_parent->rect, current_scissor_rect);
        }
        d_push_scissor_rect(scissor_rect);
        pop_scissor_rect_after_this_command = true;
      }
    }

    switch (command.commandType)
    {
      case CLAY_RENDER_COMMAND_TYPE_NONE:
      default: { InvalidCodePath(); } break;

      case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
      {
        Assert(command.userData != 0);
        if (command.userData)
        {
          // DD: Dont use command.renderData.rectangle.backgroundColor, it might be a fake color, like magenta, that was put in there just for clay to generate a render command for a box that might only have borders and no background color for us to draw the borders in the same place where we draw the background

          UI_Box* box        = (UI_Box*)command.userData;
          V4F32 color        = box->per_build_config.b_color;
          V4F32 corner_radii = __ui_v4f32_from_clay_corner_radius(command.renderData.rectangle.cornerRadius);
       
          UI_Box_key key = ui_box_key_from_str8(Str8FromC("Navigation rail setting button"));
          // if (ui_box_key_match(box->hash_table_key, key)) { BP; }

          V4F32 vertex_colors[4] = { color, color, color, color };
          F32 inner_softness     = box->per_build_config.inner_softness;
          F32 outer_softness     = box->per_build_config.outer_softness;
          F32 border_width       = box->per_build_config.border_width.x;
          V4F32 border_color     = box->per_build_config.border_color;

          // DD: Background rect
          if (!(box->per_build_config.flags & UI_Box_flag__has_background)) {
            d_add_rect_command(rect, vertex_colors, corner_radii, 0.0f, transparent(), inner_softness, outer_softness);
          }
          
          // DD: Borders
          d_add_rect_command(rect, vertex_colors, corner_radii, border_width, border_color, inner_softness, outer_softness);
        }
      } break;

      case CLAY_RENDER_COMMAND_TYPE_BORDER:
      {
        // DD: We dont conform to clay border render commands, we draw borders when we draw the main rect for clay elements
      } break;

      case CLAY_RENDER_COMMAND_TYPE_TEXT:
      {
        // DD: Not sure if we need this yet
        NotImplemented();
      } break;

      case CLAY_RENDER_COMMAND_TYPE_IMAGE:
      {
        // DD: Not sure if we need this yet
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
        Assert(command.userData != 0);
        if (command.userData)
        {
          UI_Box* box = (UI_Box*)command.userData;
          
          UI_Box_custom_draw_func* draw_func = box->per_build_config.custom_draw_extension.draw_func;
          
          Assert(draw_func);
          if (draw_func) {
            draw_func(box);
          }
        }

        // DD: Just making sure
        /*
        Assert((UI_Box*)command.renderData.custom.customData == (UI_Box*)command.userData);
        if ((UI_Box*)command.renderData.custom.customData == (UI_Box*)command.userData)
        {
          V4F32 b_color              = __ui_v4f32_from_clay_color(command.renderData.custom.backgroundColor);
          V4F32 clay_corner_r        = __ui_v4f32_from_clay_corner_radius(command.renderData.custom.cornerRadius);
          UI_Box* box_to_custom_draw = (UI_Box*)command.renderData.custom.customData;
  
          UI_Box_custom_draw_func* draw_func = box_to_custom_draw->per_build_config.custom_draw_extension.draw_func;
          void* draw_func_data               = box_to_custom_draw->per_build_config.custom_draw_extension.data_for_draw_func;

          Assert(draw_func);
          if (draw_func)
          {
            UI_Provided_data_for_custom_draw provided_data = {};
            provided_data.box              = box_to_custom_draw;
            provided_data.final_box_rect   = rect;
            provided_data.background_color = b_color;
            provided_data.corner_radii     = clay_corner_r;

            draw_func(provided_data);
          }
        }
        */

      } break;
    }
    
    if (pop_scissor_rect_after_this_command)
    {
      d_pop_scissor_rect();
    }
  }

  d_pop_scissor_rect();
}

///////////////////////////////////////////////////////////
// - Context menu embedded support
//
B32 ui_is_context_menu_with_id_open(Str8 id)
{
  UI_State* state = ui_get_state();
  UI_Box_key key = ui_box_key_from_str8(id);
  return ui_box_key_match(state->open_context_menu_box_key, key);
}

void ui_set_context_menu_key(Str8 id, V2F32 offset)
{
  UI_State* state = ui_get_state();
  UI_Box_key key = ui_box_key_from_str8(id);
  state->open_context_menu_box_key = key;
  state->open_context_menu_offset  = offset;
}

void ui_reset_context_menu()
{
  // todo: If there is no context menu in the end of the frame, then dont draw it
  UI_State* state = ui_get_state();
  state->remove_context_menu_when_closing_it = true;
}

void ui_begin_context_menu(Str8 id)
{
  UI_State* state = ui_get_state();
  UI_Box_key key = ui_box_key_from_str8(id);

  if (ui_is_context_menu_with_id_open(id))
  {
    ui_push_parent(ui_box_null());

    ui_next_width(ui_fit());
    ui_next_height(ui_fit());
    ui_next_floating_fixed_pos(state->open_context_menu_offset);
    UI_Box* context_menu_box = ui_box_make(UI_Box_flag__floating, id);

    ui_push_parent(context_menu_box);

    // DD: If we start a context menu this build that is difference from the 
    // final context menu of the previous build, then we have to reset the 
    // data for interactions 
    if (!ui_box_key_match(state->final_context_menu_key_for_prev_build, key))
    {
      Assert(state->remove_context_menu_when_closing_it == false, "DD: This is not a bug, i just wanna know if we even write code that makes this be the case");
      state->remove_context_menu_when_closing_it = false;

      for EachIndex(i, ArrayCount(state->interacted_with_box_data)) 
      {
        state->interacted_with_box_data[i] = {};
        state->interacted_with_box_data[i].box_key = ui_box_key_null();
      }
    }

  }
}

void ui_end_context_menu(Str8 id)
{
  UI_State* state = ui_get_state();
  if (ui_is_context_menu_with_id_open(id))
  {
    ui_pop_parent();
    ui_pop_parent();

    if (state->remove_context_menu_when_closing_it)
    {
      state->remove_context_menu_when_closing_it = false;
      state->open_context_menu_box_key           = ui_box_key_null();
      state->open_context_menu_offset            = V2F32{};
    }
  }
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
UI_Size ui_fit_mm(F32 min, F32 max)  { return ui_size_make(UI_Size_kind__fit, min, max); }  // DD: Not sure if these work, havent used these yet
UI_Size ui_grow_mm(F32 min, F32 max) { return ui_size_make(UI_Size_kind__grow, min, max); } // DD: Not sure if these work, havent used these yet         
UI_Size ui_fit()                     { return ui_size_make(UI_Size_kind__fit, 0.0f, 0.0f); } 
UI_Size ui_grow()                    { return ui_size_make(UI_Size_kind__grow, 0.0f, 0.0f); }         
UI_Size ui_p_of_p(F32 p)             { return ui_size_make(UI_Size_kind__percent_of_parent, p, p); }         

///////////////////////////////////////////////////////////
// - Other/Misc
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

UI_Box* ui_get_root()
{
  return ui_get_state()->current_build_root_box;
}

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
// - Stack function helpers (fixed floating stuff)
//
void ui_next_floating_fixed_pos(V2F32 pos)
{
  ui_next_floating_fixed_pos_x(pos.x);
  ui_next_floating_fixed_pos_y(pos.y);
}

void ui_next_floating_fixed_dims(V2F32 dims)
{
  ui_next_floating_fixed_dims_x(dims.x);
  ui_next_floating_fixed_dims_y(dims.y);
}

void ui_next_floating_fixed_rect(Rect rect)
{
  ui_next_floating_fixed_pos(rect.origin);
  ui_next_floating_fixed_dims(rect.dims);
}

///////////////////////////////////////////////////////////
// Private helpers
///////////////////////////////////////////////////////////

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
  clay_border.betweenChildren = {}; // DD: Not sure if we need this, so not using this yet
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
  Clay_ElementId clay_id = {};
  if (str.count != 0)
  {
    Clay_String clay_str = __ui_clay_string_from_str8(str);
    clay_id = Clay__HashString(clay_str, 0, 0);
  }
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

void __ui_box_set_to_null_mem(UI_Box* box)
{
  box->hash_table_key              = ui_box_key_null();

  box->next_in_bucket_or_free_list = &__ui_g_null_box;
  box->prev_in_bucket              = &__ui_g_null_box;

  box->per_build_config.custom_draw_extension.draw_func = __ui_custom_draw_stub_func;

  box->per_build_config.first_child  = &__ui_g_null_box;
  box->per_build_config.last_child   = &__ui_g_null_box;
  box->per_build_config.next_sibling = &__ui_g_null_box;
  box->per_build_config.prev_sibling = &__ui_g_null_box;
  box->per_build_config.parent       = &__ui_g_null_box;

  box->per_build_config.ancestor_with_no_overflow_drag_flag = &__ui_g_null_box;

  box->prev_build_parent_context_menu_key = ui_box_key_null();
  box->parent_context_menu_key            = ui_box_key_null();

  __ui_actions_set_to_null_mem(&box->actions);
}

void __ui_actions_set_to_null_mem(UI_Actions* actions)
{
  actions->box = &__ui_g_null_box;
}

void __ui_error_handler_for_clay(Clay_ErrorData errorText)
{
  BreakPoint(
    "Hey big fella."
    "I guess some went wrong since you are here,"
    "but dont worry, I believe in you."
  );
}

UI_CUSTOM_DRAW_BOX_DEF(__ui_custom_draw_stub_func)
{
  BreakPoint(
    "Hey big fella."
    "I guess some went wrong since you are here,"
    "but dont worry, I believe in you."
  );
}

UI_Box* __ui_next_box_in_box_tree_depth_first(UI_Box* box)
{
  // DD:
  // This is for depth first iteration but dont in a loop like so:
  //    for (UI_Box* box = root; !ui_box_is_null(box); box = __ui_next_box_in_box_tree_depth_first(box)) { _code_here_ }
  // 
  // In depth first we have 3 ways to go in 3 difference cases.
  // 1. If we have a first child, then the next box is that child
  // 2. If we dont have a first child, then we go to the next sibling
  // 3. If we dont have a first child and we dont have the next sibling, then we have
  //    to go to the next sibling of the first parent that has it.

  if (ui_box_is_null(box)) { return ui_box_null(); }

  UI_Box* next = ui_box_null();
  if (!ui_box_is_null(box->per_build_config.first_child))
  {
    next = box->per_build_config.first_child;
  }
  else if (!ui_box_is_null(box->per_build_config.next_sibling))
  {
    next = box->per_build_config.next_sibling;
  }
  else  
  {
    for (UI_Box* parent = box->per_build_config.parent; !ui_box_is_null(parent); parent = parent->per_build_config.parent)
    {
      if (!ui_box_is_null(parent->per_build_config.next_sibling))
      {
        next = parent->per_build_config.next_sibling;
        break;
      }
    }
  }

  return next;
}

void __ui_build_clay_element_tree_from_box_tree(UI_Box* box)
{
  if (ui_box_is_null(box)) { return; }
  UI_State* state = ui_get_state();

  if (!ui_box_key_is_null(box->hash_table_key)) 
  {
    Assert(box->per_build_config.str_for_key.count != 0); 
    Assert(box->per_build_config.is_str_for_key_fake == false); 
  }
  
  // DD: Setting up clay config from our own box state
  Clay_ElementDeclaration clay_config = {};
  {
    // DD: Making clay id
    if (box->per_build_config.str_for_key.count != 0)
    {
      clay_config.id = Clay__HashString(__ui_clay_string_from_str8(box->per_build_config.str_for_key), 0, 0);
    }
  
    clay_config.layout.sizing.width  = __ui_clay_sizing_axis_from_ui_size(box->per_build_config.size_on_axis[Axis2__x]);
    clay_config.layout.sizing.height = __ui_clay_sizing_axis_from_ui_size(box->per_build_config.size_on_axis[Axis2__y]);
    clay_config.layout.layoutDirection = (box->per_build_config.layout_direction == Axis2__x ?  CLAY_LEFT_TO_RIGHT : CLAY_TOP_TO_BOTTOM);
    
    clay_config.layout.padding  = __ui_clay_padding_from_v4f32(box->per_build_config.padding);
    clay_config.layout.childGap = (U16)box->per_build_config.child_gap;

    if (0) {}
    else if (box->per_build_config.alignment_on_x == UI_Alignment_x__left)   { clay_config.layout.childAlignment.x = CLAY_ALIGN_X_LEFT; }
    else if (box->per_build_config.alignment_on_x == UI_Alignment_x__center) { clay_config.layout.childAlignment.x = CLAY_ALIGN_X_CENTER; }
    else if (box->per_build_config.alignment_on_x == UI_Alignment_x__right)  { clay_config.layout.childAlignment.x = CLAY_ALIGN_X_RIGHT; }
  
    if (0) {}
    else if (box->per_build_config.alignment_on_y == UI_Alignment_y__top)   { clay_config.layout.childAlignment.y = CLAY_ALIGN_Y_TOP; }
    else if (box->per_build_config.alignment_on_y == UI_Alignment_y__center) { clay_config.layout.childAlignment.y = CLAY_ALIGN_Y_CENTER; }
    else if (box->per_build_config.alignment_on_y == UI_Alignment_y__bottom)  { clay_config.layout.childAlignment.y = CLAY_ALIGN_Y_BOTTOM; }
  
    // DD: Using a random color when the box doesnt have a background flag to generate 
    // a clay render comamnd of type RECT.  
    // This is needed to be able to have a draw commne for rect, which we use to draw borders,
    // for cases when we only have a box with borders and no background color.
    // This background wont be used when drawing, but the comand will be generated.
    if ( (box->per_build_config.flags & UI_Box_flag__has_background) 
      && box->per_build_config.b_color.a != 0.0f
    ) { 
      clay_config.backgroundColor = __ui_clay_color_from_v4f32(box->per_build_config.b_color);
    } else {
      clay_config.backgroundColor = __ui_clay_color_from_v4f32(magenta());
    }

    clay_config.cornerRadius    = __ui_clay_corner_radius_from_v4f32(box->per_build_config.corner_radii); 
  
    clay_config.clip.horizontal  = box->per_build_config.clip_axis[Axis2__x];
    clay_config.clip.vertical    = box->per_build_config.clip_axis[Axis2__y];
    clay_config.clip.childOffset = { box->clip_offset.x, box->clip_offset.y };
    
    clay_config.border = { __ui_clay_color_from_v4f32(box->per_build_config.border_color), __ui_clay_border_width_from_v4f32(box->per_build_config.border_width) };
    
    if (box->per_build_config.flags & UI_Box_flag__floating)
    {
      clay_config.floating.pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_CAPTURE; // DD: Not sure where i need this, so just const right now
      clay_config.floating.clipTo             = CLAY_CLIP_TO_NONE;                 // DD: Not sure where i need this, so just const right now
      clay_config.floating.offset.x = box->per_build_config.floating_fixed_pos.x; 
      clay_config.floating.offset.y = box->per_build_config.floating_fixed_pos.y; 
      // TODO: Look into this, should we set the parent to be the root box of the build when we have UI_Floating_attach_point__root ?
      //       Would that make more sense ?
      if (0) {}
      else if (box->per_build_config.floating_attach_point == UI_Floating_attach_point__parent) { clay_config.floating.attachTo = CLAY_ATTACH_TO_PARENT; }
      else if (box->per_build_config.floating_attach_point == UI_Floating_attach_point__root)   { clay_config.floating.attachTo = CLAY_ATTACH_TO_ROOT; }
      
      if (box->per_build_config.has_fixed_dims)
      {
        // TODO: Look into expand and what it does
        clay_config.layout.sizing.width  = __ui_clay_sizing_axis_from_ui_size(ui_px(box->per_build_config.floating_fixed_dims.x));
        clay_config.layout.sizing.height = __ui_clay_sizing_axis_from_ui_size(ui_px(box->per_build_config.floating_fixed_dims.y));
      }
    }
    
    if ( box->per_build_config.custom_draw_extension.draw_func != 0 
      && box->per_build_config.custom_draw_extension.draw_func != __ui_custom_draw_stub_func
    ) {
      clay_config.custom.customData = box;
    }
    
    clay_config.userData = box;
    
    // DD: We dont use these
    // config->aspectRatio = {};  
    // config->image       = {};  
  }

  // DD: Doing clay stuff to make clay ui element
  {
    Clay__OpenElement();
    Clay__ConfigureOpenElementPtr(&clay_config);

    // DD: Clay_Hovered() just returns if a mouse is over something, regardless of the fact that there might be something on top of it
    if (Clay_Hovered() && box->per_build_config.has_hover_cursor)
    {
      B32 do_it = true;

      // DD: If context menu is open then we only use the hovered cursors
      // for the boxes that are a part of the context menu
      if (!ui_box_key_is_null(state->open_context_menu_box_key))
      {
        do_it = false;
        for (UI_Box* parent = box->per_build_config.parent; ui_box_is_null(parent); parent = parent->per_build_config.parent)
        {
          if (ui_box_key_match(parent->hash_table_key, state->open_context_menu_box_key))
          {
            do_it = true;
            break;
          }
        }
      }

      if (do_it)
      {
        state->final_hover_box = box;
      }
    }

    // DD: Doing children like EPSTINE
    // DD: Doing this recursivelly and not via the recursion helper cause of clay Open/Close_Element
    for (
      UI_Box* child = box->per_build_config.first_child; 
      !ui_box_is_null(child); 
      child = child->per_build_config.next_sibling
    ) {
      __ui_build_clay_element_tree_from_box_tree(child);
    }

    Clay__CloseElement();
  }

}

void __ui_store_persistant_data_for_persistant_boxes_after_clay_done_laying_out(UI_Box* root)
{
  if (ui_box_is_null(root)) { return; }

  for (UI_Box* it_box = root; !ui_box_is_null(it_box); it_box = __ui_next_box_in_box_tree_depth_first(it_box))
  {
    if (it_box->per_build_config.str_for_key.count != 0) 
    {
      Clay_ElementId clay_id = __ui_clay_element_id_from_str8(it_box->per_build_config.str_for_key);
      Clay_ElementData clay_data = Clay_GetElementData(clay_id);
      Assert(clay_data.found);
      if (clay_data.found)
      {
        it_box->rect = __ui_rect_from_clay_bounding_box(clay_data.boundingBox);
      }
    
      if ((it_box->per_build_config.flags & UI_Box_flag__clip_x) || (it_box->per_build_config.flags & UI_Box_flag__clip_y))
      {
        Clay_ScrollContainerData clay_scroll_data = Clay_GetScrollContainerData(clay_id);
        Assert(clay_scroll_data.found);
        if (clay_scroll_data.found)
        {
          it_box->viewport_dims = __ui_v2f32_from_clay_dimensions(clay_scroll_data.scrollContainerDimensions);
          it_box->content_dims  = __ui_v2f32_from_clay_dimensions(clay_scroll_data.contentDimensions);
        }
      }
    }
  }

}

///////////////////////////////////////////////////////////
// NEW STUFF
///////////////////////////////////////////////////////////

void ui_scroll_box_with_wheel(UI_Box* box, F32 multiplier)
{
  V2F32 scroll = {};
  B32 scroll_happend = false;
  for (OS_Event* ev = os_get_frame_event_list()->first; ev; ev = ev->next)
  {
    if (ev->kind == OS_Event_kind__wheel)
    {
      scroll_happend = true;
      Axis2 axis = Axis2__y;
      if (ev->wheel_event.modifiers & OS_Event_modifier__shift) { axis = Axis2__x; }
      scroll.v[axis] += ev->wheel_event.scroll_data * multiplier;
      os_consume_frame_event(ev);
    }
  }

  UI_Box_clip_data clip_data = ui_box_clip_data_from_box(box);
  if (scroll_happend && clip_data.is_found)
  {
    for EachEnumRange(axis, Axis2, Axis2__x, Axis2__COUNT)
    {
      UI_Box_flag clip_flag = UI_Box_flag__clip_x;
      if (axis == Axis2__y) { clip_flag = UI_Box_flag__clip_y; }
      
      if (box->per_build_config.flags & clip_flag)
      {
        V2F32 current_offset = ui_box_clip_offset(box);
        V2F32 new_offset     = v2f32_add(current_offset, scroll);
        F32 max_offset       = clip_data.content_dims.v[axis] - clip_data.viewport_dims.v[axis];
        F32 min_offset       = 0.0f;
        clamp_f32_inplace(&new_offset.v[axis], -max_offset, -min_offset);
        ui_box_set_clip_offset_for_axis(box, new_offset.v[axis], axis);
      }
    }
  }

}

#endif











