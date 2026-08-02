#ifndef DRAW_API_H
#define DRAW_API_H

#include "core/core_include.h"
#include "render/render.h"
#include "font_provider/font_provider.h"

struct D_Rect_command {
  Rect  rect;
  V4F32 color_at_corners[UV__COUNT];
  V4F32 corner_radii;
  V4F32 border_color;
  F32   border_thickness;
  F32   inner_softness;
  F32   outer_softness;

  B8 is_texture;
  Rect texture_rect;
};

struct D_Rect_command_node {
  D_Rect_command rect_command;
  D_Rect_command_node* next;
};

struct D_Rect_command_batch {
  // Batch level settings that come from setting stacks
  R_Blend_kind blend_kind;                   
  R_Handle     render_target;                       
  Rect         scissor_rect;                 
  R_Fill_mode  fill_mode;   
  
  // Batch level settings that have to be set manually
  R_Handle     opt_texture; 

  D_Rect_command_node* first_command_node;           
  D_Rect_command_node* last_command_node;            
  U64 node_count;                        
};

struct D_Rect_command_batch_node {
  D_Rect_command_batch batch;
  D_Rect_command_batch_node* next;
};

struct D_Rect_command_batch_list {
  D_Rect_command_batch_node* first;
  D_Rect_command_batch_node* last;
  U64 count;
};

#include "draw/draw_stack_macros.h"

__D_STACK_DATA_TABLE_EXPANSION(__D_STACK_DEFINE_STACK_STRUCT)

struct D_State {
  Arena* state_arena;
  
  Arena* arena_for_draw_commands;
  D_Rect_command_batch_list rect_batch_list;
 
  // All the stacks
  struct {
    #define EXPANSION(Stack_type_name, inner_data_type, var_name_inside_state, ...) Stack_type_name var_name_inside_state;
    __D_STACK_DATA_TABLE_EXPANSION(EXPANSION)
    #undef EXPANSTION
  } stacks;

  // TODO: Add debug data to this layer for debug stuff
};

// - State variables
extern global D_State* __d_g_state;

// - State accessor
D_State* d_get_state();
void     d_set_state(D_State* state);

// - State
void d_init();
void d_release();

// - Batching scope
void d_begin_batching(R_Handle handle) ;
void d_end_batching();

// - Draw calls 
void d_draw_rect(Rect rect, V4F32 color);
void d_draw_circle(V2F32 center, F32 r, V4F32 color, F32 softness);
void d_draw_rect_pro(Rect rect, V4F32 color_at_corners[RectEdge__COUNT], V4F32 corner_radii,  F32 border_thickness, V4F32 border_color, F32 inner_softness, F32 outer_softness);
//
void d_draw_texture(R_Handle texture, V2F32 pos);
void d_draw_texture_pro(R_Handle texture, Rect dest_rect, Rect source_rect, V4F32 tint);
//
void d_draw_text(Str8 text, FP_Font font, F32 font_size, V2F32 pos, V4F32 color);
void d_draw_text_f(const char* fmt, FP_Font font, F32 font_size, V2F32 pos, V4F32 color, ...);

// - State getters
D_Rect_command_batch_list* d_get_batch_list();

// - Push/Pops 
__D_STACK_DATA_TABLE_EXPANSION(__D_STACK_DECLARE_PUSH_FUNC)
__D_STACK_DATA_TABLE_EXPANSION(__D_STACK_DECLARE_POP_FUNC)
__D_STACK_DATA_TABLE_EXPANSION(__D_STACK_DECLARE_TOP_FUNC)
__D_STACK_DATA_TABLE_EXPANSION(__D_STACK_DECLARE_HAS_NON_DEFAULT)

#define D_BlendKind(blend_kind) DeferLoop(d_push_blend_kind(blend_kind), d_pop_blend_kind())
#define D_RenderTarget(target)  DeferLoop(d_push_render_target(target), d_pop_render_target())
#define D_ScissorRect(rect)     DeferLoop(d_push_scissor_rect(rect), d_pop_scissor_rect())
#define D_FillMode(fill_mode)   DeferLoop(d_push_fill_mode(fill_mode), d_pop_fill_mode())

///////////////////////////////////////////////////////////
// Private helpers
///////////////////////////////////////////////////////////

// - Low level draw commands that know about how the shader works 
void __d_add_rect_texture_command(D_Rect_command command, R_Handle opt_texture);

D_Rect_command_batch* __d__get__or__make_and_get__new_batch(
  R_Handle     render_target,                       
  Rect         scissor_rect,                 
  R_Blend_kind blend_kind,                   
  R_Fill_mode  fill_mode,                       
  R_Handle     opt_texture
);

#endif