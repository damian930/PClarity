#ifndef DRAW_CPP
#define DRAW_CPP

#include "render/render.h"
#include "render/render.cpp"

#include "font_provider/font_provider.h"
#include "font_provider/font_provider.cpp"

#include "draw/draw.h"

///////////////////////////////////////////////////////////
// - State accessor 
//
global D_State* __d_g_state = 0;

///////////////////////////////////////////////////////////
// - State accessor 
//
D_State* d_get_state() { return __d_g_state; }

void d_set_state(D_State* state) { __d_g_state = state; }

///////////////////////////////////////////////////////////
// - State 
//
void d_init()
{
  Arena* state_arena = arena_alloc(Kilobytes(8));
  __d_g_state = ArenaPush(state_arena, D_State);
  __d_g_state->state_arena = state_arena;

  __d_g_state->arena_for_draw_commands = arena_alloc(Megabytes(64));
}

void d_release() 
{ 
  arena_release(&__d_g_state->arena_for_draw_commands);
  arena_release(&__d_g_state->state_arena);
  __d_g_state = 0;
}

///////////////////////////////////////////////////////////
// - Batching scope
//
void d_begin_batching(R_Handle handle) 
{ 
  ProfBeginFunc();

  D_State* state = d_get_state();
  
  state->rect_batch_list = D_Rect_command_batch_list{};
  arena_clear(state->arena_for_draw_commands); 

  ProfGroupF("Resetting all the stacks")
  {
    #define DRAW_RESET_STACKS(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, ...) \
            state->stacks.var_name_inside_state = {}; \
            state->stacks.var_name_inside_state.default_value = default_expr; 
    __D_STACK_DATA_TABLE_EXPANSION(DRAW_RESET_STACKS)
    #undef DRAW_RESET_STACKS
  }

  // TODO: I am not a 100% fan o fthis here, but fine
  d_push_render_target(handle);

  ProfEndGroup();
}

void d_end_batching() 
{ 
  // DD: Nothing here
}

///////////////////////////////////////////////////////////
// - Draw calls
//
void d_draw_rect(Rect rect, V4F32 color)
{
  // TODO: Use  d_draw_rect_pro here
  D_Rect_command command = {};
  command.rect = rect;
  for EachIndex(i, ArrayCount(command.color_at_corners)) { command.color_at_corners[i] = color; }

  __d_add_rect_texture_command(command, r_handle_zero());
}

void d_draw_circle(V2F32 center, F32 r, V4F32 color, F32 softness)
{
  D_Rect_command command = {};  
  command.rect           = rect_from_center(center, v2f32(r, r));
  command.outer_softness = softness;

  command.color_at_corners[UV__top_left]     = color; 
  command.color_at_corners[UV__top_right]    = color; 
  command.color_at_corners[UV__bottom_left]  = color; 
  command.color_at_corners[UV__bottom_right] = color; 

  command.corner_radii.v[UV__top_left]     = r; 
  command.corner_radii.v[UV__top_right]    = r; 
  command.corner_radii.v[UV__bottom_left]  = r; 
  command.corner_radii.v[UV__bottom_right] = r;

  __d_add_rect_texture_command(command, r_handle_zero());
}

void d_draw_rect_pro(Rect rect, V4F32 color_at_corners[UV__COUNT], V4F32 corner_radii, F32 border_thickness, V4F32 border_color, F32 inner_softness, F32 outer_softness)
{
  D_Rect_command command = {};
  command.rect             = rect;
  command.corner_radii     = corner_radii;
  command.border_thickness = border_thickness;
  command.border_color     = border_color;
  command.inner_softness   = inner_softness;
  command.outer_softness   = outer_softness;
  command.color_at_corners[UV__top_left]     = color_at_corners[UV__top_left]; 
  command.color_at_corners[UV__top_right]    = color_at_corners[UV__top_right]; 
  command.color_at_corners[UV__bottom_left]  = color_at_corners[UV__bottom_left]; 
  command.color_at_corners[UV__bottom_right] = color_at_corners[UV__bottom_right]; 

  __d_add_rect_texture_command(command, r_handle_zero());
}

void d_draw_texture(R_Handle texture, V2F32 pos)
{
  Rect dest_rect   = rect_make_v(pos, r_get_handle_dims(texture));
  Rect source_rect = rect_make_v(v2f32(0, 0), r_get_handle_dims(texture));
  d_draw_texture_pro(texture, dest_rect, source_rect, white());
}

void d_draw_texture_pro(R_Handle texture, Rect dest_rect, Rect source_rect, V4F32 tint)
{
  D_Rect_command command = {};  
  command.is_texture   = true;  
  command.rect         = dest_rect;
  command.texture_rect = source_rect;
  // TODO: Use tint
  
  __d_add_rect_texture_command(command, texture);
}

void d_draw_text(Str8 text, FP_Font font, F32 font_size, V2F32 pos, V4F32 color)
{
  F32 scale_factor = font_size / font.size;
  
  F32 origin_y = pos.y + (font.ascent * scale_factor);
  F32 x_offset = 0.0f;

  for (U64 ch_index = 0; ch_index < text.count; ch_index += 1)
  {
    U8 ch = text.data[ch_index];
    FP_Codepoint_data glyph_data = fp_get_glyph_data(font, ch); 

    F32 origin_x = pos.x + x_offset;

    // Just puttin them 1 next to another
    Rect dest_rect = {};
    dest_rect.x      = origin_x + (glyph_data.bearing_x * scale_factor);
    dest_rect.y      = origin_y - (glyph_data.bearing_y * scale_factor);
    dest_rect.width  = (glyph_data.rect_on_atlas.width * scale_factor);
    dest_rect.height = (glyph_data.rect_on_atlas.height * scale_factor);
    
    d_draw_texture_pro(font.atlas_texture, dest_rect, glyph_data.rect_on_atlas, color);

    F32 advance = (glyph_data.advance * scale_factor);
    if (ch_index < text.count - 1)
    {
      FP_Kerning_entry entry = fp_get_kerning(font, ch, text.data[ch_index + 1]);
      if (!IsMemZero(entry)) { advance += (entry.advance * scale_factor); }
    } 
    x_offset += advance; 
  }

  #if DEBUG_MODE
  { // Making sure that the x here is the same as in fp to make sure that that we dont do any stupid mistackes
    V2F32 fp_text_dims = fp_measure_text(text, font, font_size);
    Assert(0.001f > abs_f32(x_offset - fp_text_dims.x));
  }
  #endif
}

void d_draw_text_f(const char* fmt, FP_Font font, F32 font_size, V2F32 pos, V4F32 color, ...)
{
  Scratch scratch = get_scratch(0, 0);
  va_list valist;
  va_start(valist, color);
  Str8 str = str8_valist(scratch.arena, fmt, valist);
  d_draw_text(str, font, font_size, pos, color);
  va_end(valist);
  end_scratch(&scratch);
}


///////////////////////////////////////////////////////////
// - State getters
//
D_Rect_command_batch_list* d_get_batch_list()
{
  return &d_get_state()->rect_batch_list;
}

///////////////////////////////////////////////////////////
// - Push/Pops
//
__D_STACK_DATA_TABLE_EXPANSION(__D_STACK_DEFINE_PUSH_FUNC)
__D_STACK_DATA_TABLE_EXPANSION(__D_STACK_DEFINE_POP_FUNC)
__D_STACK_DATA_TABLE_EXPANSION(__D_STACK_DEFINE_TOP_FUNC)
__D_STACK_DATA_TABLE_EXPANSION(__D_STACK_DEFINE_HAS_NON_DEFAULT)

///////////////////////////////////////////////////////////
// Private helpers
///////////////////////////////////////////////////////////

void __d_add_rect_texture_command(D_Rect_command command, R_Handle opt_texture)
{
  D_State* state = d_get_state();

  R_Blend_kind blend_kind = d_top_blend_kind();
  R_Handle render_target  = d_top_render_target();
  Rect scissor_rect       = d_top_scissor_rect();
  R_Fill_mode fill_mode   = d_top_fill_mode();

  D_Rect_command_batch* batch = __d__get__or__make_and_get__new_batch(render_target, scissor_rect, blend_kind, fill_mode, opt_texture);

  Arena* arena = state->arena_for_draw_commands;
  D_Rect_command_node* new_command_node = ArenaPush(arena, D_Rect_command_node);
  new_command_node->rect_command = command;
 
  QueuePushBack_Ex(batch, new_command_node, first_command_node, last_command_node, next, is_zero_pointer, 0);
  batch->node_count += 1;
}

D_Rect_command_batch* __d__get__or__make_and_get__new_batch(
  R_Handle     render_target,                       
  Rect         scissor_rect,                 
  R_Blend_kind blend_kind,                   
  R_Fill_mode  fill_mode,                       
  R_Handle     opt_texture
) {
  D_State* state = d_get_state();

  D_Rect_command_batch* batch = 0;
  {
    D_Rect_command_batch_node* batch_node = state->rect_batch_list.last;
    if (batch_node)
    {
      batch = &batch_node->batch;
    }
  }
  
  B32 have_to_make_a_new_batch = false;
  if ( batch == 0
    || !r_handle_match(batch->render_target, render_target)
    || !rect_match(batch->scissor_rect, scissor_rect) 
    || !(batch->blend_kind == blend_kind)
    || !(batch->fill_mode == fill_mode)
    || !r_handle_match(batch->opt_texture, opt_texture) 
  ) { 
    have_to_make_a_new_batch = true;
  }

  if (have_to_make_a_new_batch)
  {
    Arena* arena = state->arena_for_draw_commands;
    D_Rect_command_batch_node* new_batch_node = ArenaPush(arena, D_Rect_command_batch_node);
    D_Rect_command_batch* new_batch = &new_batch_node->batch;
    new_batch->render_target = render_target;                       
    new_batch->scissor_rect  = scissor_rect;                 
    new_batch->blend_kind    = blend_kind;                   
    new_batch->fill_mode     = fill_mode;                       
    new_batch->opt_texture   = opt_texture; 
  
    QueuePushBack(&state->rect_batch_list, new_batch_node);
    state->rect_batch_list.count += 1;
  
    batch = new_batch;
  }

  return batch;
}

#endif