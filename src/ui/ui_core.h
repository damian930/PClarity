#ifndef __UI_H
#define __UI_H

#include "__third_party/clay/clay_code_after_0.14_release__July_30_2026/clay.h"

#include "core/core_include.h"
#include "font_provider/font_provider.h"
#include "draw/draw.h"

struct UI_Box; 

enum UI_Size_kind {
  UI_Size_kind__px,
  UI_Size_kind__fit,
  UI_Size_kind__percent_of_parent, 
  UI_Size_kind__grow, 
};

struct UI_Size {
  UI_Size_kind kind;
  F32 value1;
  F32 value2;
};

// TODO: Reset the ( << -value_) here inside the enums 
enum UI_Box_flag : U32 {
  UI_Box_flag__NONE                = (0 << 0),

  UI_Box_flag__has_background      = (1 << 1),
  UI_Box_flag__has_padding         = (1 << 2),
  UI_Box_flag__has_child_gap       = (1 << 3),
  UI_Box_flag__has_rounded_corners = (1 << 4),
  UI_Box_flag__has_borders         = (1 << 5),
  
  // Clips the box contents on axis. Clipping is not the same as just not drawing. 
  // Clipping changes the interactive zone of boxes. 
  // Not drawing would just not draw a part of the box, but the box would still
  // act as if it is full sized, so all inputs would still go thought, even thought
  // a part of the box is not drawn. Clip doesnt allow that. If a box is 
  // a child of a clip box and if outisde of its parent's on screen bounding box
  // the inputs to it dont go thought, since they are clipped out, both for the user
  // on the screen and for the ui logic. 
  UI_Box_flag__clip_x = (1 << 7), 
  UI_Box_flag__clip_y = (1 << 8), 

  UI_Box_flag__dont_draw_overflow = (1 << 9),  

  UI_Box_flag__floating = (1 << 10),  
  
  UI_Box_flag__left_clickable  = (1 << 11),  
  UI_Box_flag__right_clickable = (1 << 12),  

  // DD, Todo: These are new flags, 
  // UI_Box_flag__use_automatic_wheel_scroll = (1 << 13), 

  // ============================================

  UI_Box_flag__clickable = UI_Box_flag__left_clickable|UI_Box_flag__right_clickable,

  UI_Box_flag__has_padded_border = UI_Box_flag__has_padding|UI_Box_flag__has_borders,
  UI_Box_flag__clip          = UI_Box_flag__clip_x|UI_Box_flag__clip_y, 
};
typedef U32 UI_Box_flags;

// TODO: These might be a flag and not a separate type
enum UI_Alignment_x : U32 {
  UI_Alignment_x__left,
  UI_Alignment_x__center,
  UI_Alignment_x__right,
  UI_Alignment_x__COUNT,
};

// TODO: These might be a flag and not a separate type
enum UI_Alignment_y : U32 {
  UI_Alignment_y__top,
  UI_Alignment_y__center,
  UI_Alignment_y__bottom,
  UI_Alignment_y__COUNT,
};

// TODO: These might be a flag and not a separate type
enum UI_Floating_attach_point : U32 {
  UI_Floating_attach_point__parent,
  UI_Floating_attach_point__root,
  UI_Floating_attach_point__COUNT,
};

#define UI_CUSTOM_DRAW_BOX_DEF(name) void name(UI_Box* box)
typedef UI_CUSTOM_DRAW_BOX_DEF(UI_Box_custom_draw_func);

enum UI_Mouse_button : U32 {
  UI_Mouse_button__left,
  UI_Mouse_button__right,
  UI_Mouse_button__COUNT,
};

struct UI_Actions {
  // Lower level actions
  B32 is_hovered;                                       
  //
  B32 is_left_down;
  B32 was_left_down;
  B32 left_left_box_while_was_down;
  //
  B32 is_right_down;
  B32 was_right_down;
  B32 right_left_box_while_was_down;

  // Composed for quick use
  B32 is_left_clicked;
  B32 left_went_down;
  B32 left_went_up;
  //
  B32 is_right_clicked;
  B32 right_went_down;
  B32 right_went_up;
  //
  B32 is_down;
  B32 was_down;
  B32 left_box_while_was_down;
  //
  B32 is_clicked;
  B32 went_down;
  B32 went_up;

  // Misc
  V2F32 mouse_pos_when_went_down;
  UI_Box* box;
};

struct UI_Box_key {
  U64 v;
};

struct UI_Box {
  // Main data that identifies a box. Always present after making of the box or its retrival
  U64        generation_when_first_created;
  U64        generation_when_last_created;
  UI_Box_key hash_table_key;

  // Persistent across builds (This is the thing that persists the boxes between builds)
  UI_Box* next_in_bucket_or_free_list;
  UI_Box* prev_in_bucket;

  // Data that we get only after a build is done
  Rect  rect;
  V2F32 viewport_dims;
  V2F32 content_dims;

  // Stuff from prev build
  struct {
    UI_Box_flags flags;
    V4F32        padding;
  } data_from_previous_build;

  // Per build config
  struct {
    Str8 str_for_key;
    B32  is_str_for_key_fake;

    UI_Box_flags flags;
    UI_Size      size_on_axis[Axis2__COUNT];
    Axis2        layout_direction;
    //
    V4F32          padding;
    F32            child_gap;
    UI_Alignment_x alignment_on_x;
    UI_Alignment_y alignment_on_y;
    V4F32          b_color;
    V4F32          corner_radii;
    B32            clip_axis[Axis2__COUNT];
    V4F32          border_width;
    V4F32          border_color;
    V2F32          floating_fixed_pos;
    F32 outer_softness;
    F32 inner_softness;
    //
    // TODO: This should be a flag also
    B32   has_fixed_dims;
    V2F32 floating_fixed_dims;
    //
    UI_Floating_attach_point floating_attach_point;
    //
    // TODO: THis should be a flag
    B32       has_hover_cursor;
    OS_Cursor hover_cursor;

    struct {
      UI_Box_custom_draw_func* draw_func; 
      void*                    data_for_draw_func;
    } custom_draw_extension;

    struct {
      Str8    text;
      F32     font_size; // DD: This is the font size to draw the text in, right now we use manual scaling, so the size that the font was generated for is not used 
      FP_Font font;
      V4F32   font_color;
    } text_extension;
  
    // Per build ui tree links
    UI_Box* first_child;
    UI_Box* last_child;
    UI_Box* next_sibling;
    UI_Box* prev_sibling;
    UI_Box* parent;
    U64     children_count;
  
    // Additional data that we have to keep for drawing since clay linearises the drawing
    UI_Box* ancestor_with_no_overflow_drag_flag;

  } per_build_config; 

  ///////////////////////////////////////////////////////////
  // Other/Misc
  //
  Data_buffer dynamic_drag_memory;

  V2F32 clip_offset;
  struct {
    B32   is_present;
    V2F32 offset;
  } defered_clip_offset;

  B32        actions_present; 
  UI_Actions actions;  

  UI_Box_key prev_build_parent_context_menu_key;
  UI_Box_key parent_context_menu_key;
};

struct UI_Box_data {
  B32  is_found;
  Rect rect;       // Rect for   the box
  Rect inner_rect; // Rect after we  remove padding, this is the rect where the children are placed
};

struct UI_Box_clip_data {
  B32   is_found;
  V2F32 viewport_dims;
  V2F32 content_dims;
  V2F32 offset;
};

// DD: 
// This is separated into a separete file just cause its easier to have
// macros be there, i think.
// Some of those macros need some types from above, so we include it here.
#include "ui/ui_stack_macros.h" 

// Definiting stack structs
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DEFINE_STACK_STRUCTS)

struct UI_Box_list {
  UI_Box* first;
  UI_Box* last;
  U64     count;
};

struct UI_State {
  Arena* state_arena;
  Arena* arena_for_clay; 

  Clay_Context* clay_context;

  U64    build_generation;
  Arena* build_arenas[2]; // Todo: do we need 2 of these

  // Hash table for persistant boxes
  UI_Box_list hash_table_buckets[64];
  
  // Free list for persistant boxes
  UI_Box* first_free_box;
  U64     count_of_free_boxes;

  V2F32 mouse_pos_for_this_build;
  V2F32 window_dims_for_this_build;
  V2F32 mouse_pos_for_prev_build;
  
  U64 this_build_box_count;
  U64 last_build_box_count;

  // Result of ui build 
  Clay_RenderCommandArray render_commands_as_result_of_ui_build; 
  UI_Box* final_hover_box;
  
  // Persistant, cross-frame state needed for mouse interactions  
  struct {
    UI_Box_key box_key;
    B32        is_mouse_down;
    B32        did_mouse_leave_box_while_was_down;
    V2F32      pos_when_mouse_went_down;
  } interacted_with_box_data[UI_Mouse_button__COUNT];

  UI_Box* current_build_root_box; 

  // All the stacks
  struct {
    #define EXPANSION(Stack_type_name, inner_data_type, var_name_inside_state, ...) Stack_type_name var_name_inside_state;
    __UI_STACK_DATA_TABLE_EXPANSION(EXPANSION)
    #undef EXPANSTION
  } stacks;

  UI_Box sentinel_zero_box;

  ///////////////////////////////////////////////////////////
  // Other/Misc
  //
  UI_Box_key final_context_menu_key_for_prev_build;

  UI_Box_key open_context_menu_box_key; 
  V2F32 open_context_menu_offset;
  B32 remove_context_menu_when_closing_it;
};

// - State variables
extern global UI_State* __ui_g_state;

// - State accessors
UI_State* ui_get_state();
void      ui_set_state(UI_State* context);

// - State
void ui_init();
void ui_release();

// - UI building
void ui_begin_build(V2F32 window_dims, V2F32 mouse_pos, FP_Font default_font);
void ui_end_build();
#define UI_Build(window_dims, mouse_pos, default_font) DeferLoop(ui_begin_build(window_dims, mouse_pos, default_font), ui_end_build())

// - Box making
UI_Box* ui_box_make(UI_Box_flags flags, Str8 id_and_text);
UI_Box* ui_box_make_f(UI_Box_flags flags, const char* fmt, ...);

// - Box extension
void ui_extend_box_with_custom_draw_function(UI_Box* box, UI_Box_custom_draw_func* custom_draw, void* data);
void ui_extend_box_with_text(UI_Box* box, Str8 str);

// - Box data
UI_Box_data ui_box_data_from_box(UI_Box* box);
UI_Box_data ui_box_data_from_id(Str8 id);

// - Box clip data
UI_Box_clip_data ui_box_clip_data_from_box(UI_Box* box);
UI_Box_clip_data ui_box_clip_data_from_id(Str8 id);

// - Box actions 
UI_Actions ui_actions_from_box(UI_Box* box);
UI_Actions ui_actions_from_id(Str8 id);

// - Box clip offset
V2F32 ui_box_clip_offset(UI_Box* box);
V2F32 ui_box_clip_offset_by_id(Str8 id); 

// - Box key stuff
UI_Box_key ui_box_key_null();
B32        ui_box_key_match(UI_Box_key key, UI_Box_key other);
B32        ui_box_key_is_null(UI_Box_key key);
UI_Box_key ui_box_key_from_str8(Str8 str);
UI_Box*    ui_box_from_key(UI_Box_key key);

// - Box drag memory
Data_buffer* ui_box_drag_buffer(UI_Box* box);   
Data_buffer* ui_box_drag_buffer_by_id(Str8 id); 
Data_buffer* ui_box_drag_buffer_alloc(UI_Box* box, U64 size_to_alloc);
Data_buffer* ui_box_drag_buffer_alloc_by_id(Str8 id, U64 size_to_alloc);
void         ui_box_drag_buffer_release(UI_Box* box);
void         ui_box_drag_buffer_release_by_id(Str8 id);

// - Box setters (clip offset) 
void ui_box_set_clip_offset_for_axis(UI_Box* box, F32 clip_offset, Axis2 axis);
void ui_box_set_clip_offset_for_axis_by_id(Str8 id, F32 clip_offset, Axis2 axis);
void ui_box_set_clip_offset_y(UI_Box* box, F32 offset);
void ui_box_set_clip_offset_x(UI_Box* box, F32 offset);

// - Null box
B32     ui_box_is_null(UI_Box* box);
UI_Box* ui_box_null();

// - UI drawing
void ui_draw();

// - Context menu embedded support
B32  ui_is_context_menu_with_id_open(Str8 id);
void ui_set_context_menu_key(Str8 id, V2F32 offset);
void ui_reset_context_menu();
void ui_begin_context_menu(Str8 id);
void ui_end_context_menu(Str8 id); 
#define UI_ContextMenu(id) if (ui_is_context_menu_with_id_open(id)) DeferLoop(ui_begin_context_menu(id), ui_end_context_menu(id))

// - Size makers 
UI_Size ui_size_make(UI_Size_kind kind, F32 value1, F32 value2);
UI_Size ui_px(F32 value);                 
UI_Size ui_rem(F32 scale);                 
UI_Size ui_fit_mm(F32 min, F32 max);      
UI_Size ui_grow_mm(F32 min, F32 max);     
UI_Size ui_fit();                         
UI_Size ui_grow();                        
UI_Size ui_p_of_p(F32 p);                 

// - Other/Misc
U64     ui_get_build_generation();
Arena*  ui_get_build_arena();
V2F32   ui_get_mouse_pos();
V2F32   ui_get_prev_mouse_pos();
UI_Box* ui_get_root(); 

// - Stack funtions and helpers
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DECLARE_PUSH_FUNC)
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DECLARE_POP_FUNC)
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DECLARE_AUTO_POP_FUNC)
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DECLARE_GET_FUNC)
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DECLARE_SET_NEXT_FUNC)

// - Stack function helpers (padding)
V4F32 ui_top_padding();
void  ui_next_padding(F32 padding);
void  ui_push_padding(F32 padding); 
void  ui_pop_padding(); 
void  ui_next_padding_ex(F32 left, F32 right, F32 top, F32 down);

// - Stack function helpers (sizing)
void ui_next_width(UI_Size size);
void ui_next_height(UI_Size size);
void ui_next_size_axis(Axis2 axis, UI_Size size);

// - Stack function helpers (background color)
V4F32 ui_top_b_color();
void  ui_next_b_color(V4F32 color);
void  ui_push_b_color(V4F32 color);
void  ui_pop_b_color();

// - Stack function helpers (corner radius)
V4F32 ui_top_corner_radius();
void  ui_next_corner_r(F32 r);
void  ui_push_corner_r(F32 r);
void  ui_pop_corner_r();

// - Stack function helpers (border width)
V4F32 ui_top_border_width();
void  ui_next_border_width(F32 border);
void  ui_push_border_width(F32 border);
void  ui_pop_border_width();

// - Stack function helpers (border)
void ui_next_border(F32 width, V4F32 color);
void ui_push_border(F32 width, V4F32 color);
void ui_pop_border();

// - Stack function helpers (padded border)
void ui_next_padded_border(F32 width, V4F32 color);
void ui_push_padded_border(F32 width, V4F32 color);
void ui_pop_padded_border();

// - Stack function helpers (layout)
void ui_next_layout_x();
void ui_next_layout_y();

// - Stack function helpers (fixed floating stuff)
void ui_next_floating_fixed_pos(V2F32 pos);
void ui_next_floating_fixed_dims(V2F32 dims);
void ui_next_floating_fixed_rect(Rect rect);

// - Macros for automatic stack pushing and popping
// Damian: I would like to do something like that, have a macro that generates macros, but that is not possible in c/cpp.
//         This is exactly the reason why ryan has his own table tool generation. Also his tool outputs files that you
//         then can go read the code for, which is the issue with macros - you dont really see the final code. 
//         So gonna have to define the DeferLoop macros for push and pops manually
// 
// #define __UI_STACK_DEFINE_DEFER_PUSH_POP_MACROS(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, set_next_func_name, pop_func_name, auto_pop_func_name, get_top_func_name, stack_arr_capacity) \
//   #define defer_push_pop_macro_name(v) DeferLoop(push_func_name(v), pop_func_name())
// #undef __UI_STACK_DEFINE_DEFER_PUSH_POP_MACROS
//
#define UI_SizeX(v)                   DeferLoop(ui_push_size_x(v),                     ui_pop_size_x())
#define UI_SizeY(v)                   DeferLoop(ui_push_size_y(v),                     ui_pop_size_y())
#define UI_Childgap(v)                DeferLoop(ui_push_child_gap(v),                  ui_pop_child_gap())
#define UI_PaddingLeft(v)             DeferLoop(ui_push_padding_left(v),               ui_pop_padding_left())
#define UI_PaddingTop(v)              DeferLoop(ui_push_padding_top(v),                ui_pop_padding_top())
#define UI_PaddingRight(v)            DeferLoop(ui_push_padding_right(v),              ui_pop_padding_right())
#define UI_PaddingBottom(v)           DeferLoop(ui_push_padding_bottom(v),             ui_pop_padding_bottom())
#define UI_Layout(v)                  DeferLoop(ui_push_layout(v),                     ui_pop_layout())
#define UI_BColor(v)                  DeferLoop(ui_push_background_color(v),           ui_pop_background_color())
#define UI_CornerRadiusTopLeft(v)     DeferLoop(ui_push_corner_radius_top_left(v),     ui_pop_corner_radius_top_left())
#define UI_CornerRadiusTopRight(v)    DeferLoop(ui_push_corner_radius_top_right(v),    ui_pop_corner_radius_top_right())
#define UI_CornerRadiusBottomRight(v) DeferLoop(ui_push_corner_radius_bottom_right(v), ui_pop_corner_radius_bottom_right())
#define UI_CornerRadiusBottomLeft(v)  DeferLoop(ui_push_corner_radius_bottom_left(v),  ui_pop_corner_radius_bottom_left())
#define UI_BorderColor(v)             DeferLoop(ui_push_border_color(v),               ui_pop_border_color())
#define UI_BorderLeft(v)              DeferLoop(ui_push_border_left(v),                ui_pop_border_left())
#define UI_BorderRight(v)             DeferLoop(ui_push_border_right(v),               ui_pop_border_right())
#define UI_BorderTop(v)               DeferLoop(ui_push_border_top(v),                 ui_pop_border_top())
#define UI_BorderBottom(v)            DeferLoop(ui_push_border_bottom(v),              ui_pop_border_bottom())
#define UI_Parent(v)                  DeferLoop(ui_push_parent(v),                     ui_pop_parent())
#define UI_Font(v)                    DeferLoop(ui_push_font(v),                       ui_pop_font())
#define UI_FontSize(v)                DeferLoop(ui_push_font_size(v),                  ui_pop_font_size())
#define UI_FontColor(v)               DeferLoop(ui_push_font_color(v),                 ui_pop_font_color())
#define UI_AlignmentX(v)              DeferLoop(ui_push_alignment_x(v),                ui_pop_alignment_x())
#define UI_AlignmentY(v)              DeferLoop(ui_push_alignment_y(v),                ui_pop_alignment_y())
#define UI_HoverCursor(v)             DeferLoop(ui_push_hover_cursor(v),               ui_pop_hover_cursor())
#define UI_PaddedBorder(v, c)         DeferLoop(ui_push_padded_border(v, c),           ui_pop_padded_border)
#define UI_Border(v, c)               DeferLoop(ui_push_border(v, c),                  ui_pop_border())
#define UI_Padding(v)                 DeferLoop(ui_push_padding(v),                    ui_pop_padding())
//
#define UI_Width(v) UI_SizeX(v)
#define UI_Height(v) UI_SizeY(v)

///////////////////////////////////////////////////////////
// Private helpers
///////////////////////////////////////////////////////////

// - Helpers to wrap around clay
Clay_SizingAxis   __ui_clay_sizing_axis_from_ui_size (UI_Size ui_size);
Clay_Padding      __ui_clay_padding_from_v4f32       (V4F32 padding);
V4F32             __ui_v4f32_from_clay_padding       (Clay_Padding clay_padding);
Clay_Color        __ui_clay_color_from_v4f32         (V4F32 color);
V4F32             __ui_v4f32_from_clay_color         (Clay_Color clay_color);
Clay_BorderWidth  __ui_clay_border_width_from_v4f32  (V4F32 border);
V4F32             __ui_v4f32_from_clay_border_width  (Clay_BorderWidth clay_border_width);
Str8              __ui_str8_from_clay_string         (Clay_String clay_string);
Clay_String       __ui_clay_string_from_str8         (Str8 str);
Rect              __ui_rect_from_clay_bounding_box   (Clay_BoundingBox bbox);
Clay_BoundingBox  __ui_clay_bounding_box_from_rect   (Rect rect);
V4F32             __ui_v4f32_from_clay_corner_radius (Clay_CornerRadius clay_crs);
Clay_CornerRadius __ui_clay_corner_radius_from_v4f32 (V4F32 vec);
Clay_ElementId    __ui_clay_element_id_from_str8     (Str8 str);
V2F32             __ui_v2f32_from_clay_dimensions    (Clay_Dimensions clay_dims);
Clay_Dimensions   __ui_clay_dimensions_from_v2f32    (V2F32 vec);

void __ui_box_set_to_null_mem(UI_Box* box);
void __ui_actions_set_to_null_mem(UI_Actions* actions);

void __ui_error_handler_for_clay(Clay_ErrorData errorText);
UI_CUSTOM_DRAW_BOX_DEF(__ui_custom_draw_stub_func); 

UI_Box* __ui_next_box_in_box_tree_depth_first(UI_Box* box);

void __ui_build_clay_element_tree_from_box_tree(UI_Box* box);
void __ui_store_persistant_data_for_persistant_boxes_after_clay_done_laying_out(UI_Box* root);

#endif






