#ifndef __UI_H
#define __UI_H

#include "core/core_include.h"
#include "__third_party/clay/clay.h"

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

enum UI_Box_flag : U32 {
  UI_Box_flag__NONE                = (0 << 0),

  UI_Box_flag__has_background      = (1 << 1),
  UI_Box_flag__has_padding         = (1 << 2),
  UI_Box_flag__has_child_gap       = (1 << 3),
  UI_Box_flag__has_rounded_corners = (1 << 4),
  UI_Box_flag__has_borders         = (1 << 5),
  
  UI_Box_flag__has_text_contents   = (1 << 6),

  // Floating doesnt add to the size of its parent and is not a part of the normal layout flow
  UI_Box_flag__floating_x = (1 << 7), 
  UI_Box_flag__floating_y = (1 << 8), 

  // Clips the box contents on axis. Clipping is not the same as just not drawing. 
  // Clipping changes the interactive zone of boxes. 
  // Not drawing would just not draw a part of the box, but the box would still
  // act as if it is full sized, so all inputs would still go thought, even thought
  // a part of the box is not drawn. Clip doesnt allow that. If a box is 
  // a child of a clip box and if outisde of its parent's on screen bounding box
  // the inputs to it dont go thought, since they are clipped out, both for the user
  // on the screen and for the ui logic. 
  UI_Box_flag__clip_x = (1 << 9), 
  UI_Box_flag__clip_y = (1 << 10), 

  UI_Box_flag__dont_draw_overflow = (1 << 11),  

  UI_Box_flag__padded_border      = UI_Box_flag__has_padding|UI_Box_flag__has_borders,
  UI_Box_flag__floating           = UI_Box_flag__floating_x|UI_Box_flag__floating_y, 
  UI_Box_flag__clip               = UI_Box_flag__clip_x|UI_Box_flag__clip_y, 
};
typedef U32 UI_Box_flags;

enum UI_Alignment_x : U32 {
  UI_Alignment_x__left,
  UI_Alignment_x__center,
  UI_Alignment_x__right,
  UI_Alignment_x__COUNT,
};

enum UI_Alignment_y : U32 {
  UI_Alignment_y__top,
  UI_Alignment_y__center,
  UI_Alignment_y__bottom,
  UI_Alignment_y__COUNT,
};

struct UI_Box; // TODO: Move this to a better place
struct UI_Provided_data_for_custom_draw {
  UI_Box* box;
  
  // Damian: These are provided by clay directly 
  Rect final_box_rect;
  V4F32 background_color;
  V4F32 corner_radii;
};
#define UI_CUSTOM_DRAW_BOX_DEF(name) void name(UI_Provided_data_for_custom_draw provided_data)
typedef UI_CUSTOM_DRAW_BOX_DEF(UI_Box_custom_draw_func_pointer_type);

UI_CUSTOM_DRAW_BOX_DEF(__ui_custom_draw_stub_func); 

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

  // TODO: This is new data, so putting this here for now
  V2F32 mouse_pos_when_went_down;
};

struct UI_Box {
  // Our own config
  UI_Box_flags flags;

  // Clay config
  // TODO: Dont store this, just store your own data for ui box, then when making clay boxes use this. 
  //       It will make it make more sense and the bridge between ui and clay_ui way cleaner.
  Clay_ElementDeclaration clay_element_config;

  B32 has_hover_cursor;
  OS_Cursor hover_cursor;

  V2F32 clip_offset;

  struct {
    UI_Box_custom_draw_func_pointer_type* draw_func; 
    void* data_for_draw_func;
  } custom_draw_extension;

  struct {
    Str8 text;
    F32 font_size; // Damian: This is the font size to draw the text in, right now we use manual scaling, so the size that the font was generated for is not used 
    FP_Font font;
    V4F32 font_color;
  } text_extension;

  UI_Box* first_child;
  UI_Box* last_child;
  UI_Box* next_sibling;
  UI_Box* prev_sibling;
  UI_Box* parent;
  U64 children_count;

  // Damian: These are not used on the immediate box, but rather used for the future 
  //         representation of this box 
  //         (future representation is this same box in the next build)
  B32 is_updated_actions_for_this_in_the_future;
  UI_Actions actions_for_this_in_the_future;

  U64 generation;
};

// TODO: Move this to a better place
// TODO: Also redo the __UI_NULL_BOX_VALUE since it might be wrong if the order of the box field have changed since you did the macor
#define __UI_NULL_BOX_VALUE { \
  {}, \
  {}, \
  {}, \
  {}, \
  {}, \
  { \
    __ui_custom_draw_stub_func, \
    {}, \
  }, \
  { \
    {}, \
    {}, \
    {}, \
  }, \
  &__ui_g_null_box, \
  &__ui_g_null_box, \
  &__ui_g_null_box, \
  &__ui_g_null_box, \
  &__ui_g_null_box, \
  {}, \
  \
  {}, \
  {}, \
  {}, \
}
global UI_Box __ui_g_null_box = __UI_NULL_BOX_VALUE;

struct UI_Box_data {
  B32 is_found;
  Rect rect;
  Rect inner_rect;
};

// This is separated into a separete file just cause its easier to have
// macros be there, i think.
// Some of those macros need some types from above, so we include it here.
#include "ui/ui_stack_macros.h" 

// Stack structs
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DEFINE_STACK_STRUCTS)

// TODO: Move data fiels in state for better structural meaning
struct UI_State {
  // TODO: Add a counter for boxes made last build
  // TODO: Add a build counter just for debug purposes if we need to
  
  Arena* state_arena;

  // Memory for our own box tree for prev build and the current build
  U64 build_generation;
  Arena* build_arenas[2];

  // This is arena for clay internal memory
  Arena* arena_for_clay; 

  // Result of ui build
  Clay_RenderCommandArray render_commands_as_result_of_ui_build;
  UI_Box* final_hover_box;

  // Always available data
  V2F32 mouse_pos_for_this_build;
  V2F32 window_dims_for_this_build;
  V2F32 mouse_pos_for_prev_build;

  UI_Box* next_new_elements_parent_box; // Damian, TODO: What the fuck is this even

  struct {
    Clay_ElementId clay_id;
    B32 is_mouse_down;
    B32 did_mouse_leave_box_while_was_down;
    V2F32 pos_when_mouse_went_down;
  } interacted_with_box_data;

  UI_Box* current_build_root_box; // This is allocated on the current build arena 
  UI_Box* prev_build_root_box;    // This is allocated on the previous build arena
  
  struct {
    #define EXPANSION(Stack_type_name, inner_data_type, var_name_inside_state, ...) Stack_type_name var_name_inside_state;
    __UI_STACK_DATA_TABLE_EXPANSION(EXPANSION)
    #undef EXPANSTION
  } stacks;
};

// - Context variables
extern UI_State* __ui_g_state;

// - State 
UI_State* ui_get_state();
void ui_set_state(UI_State* context);
void ui_init();
void ui_release();

// - UI building
void ui_begin_build(V2F32 window_dims, V2F32 mouse_pos, FP_Font default_font);
void ui_end_build();
void __ui_build_clay_element_tree_from_box_tree(UI_Box* root);
#define UI_Build(window_dims, mouse_pos, default_font) DeferLoop(ui_begin_build(window_dims, mouse_pos, default_font), ui_end_build())

// - UI drawing
void ui_draw();

// - Box making
B32 ui_is_null_box(UI_Box* box);
UI_Box* ui_null_box();
UI_Box* ui_box_make(Str8 id_and_text, UI_Box_flags flags);
UI_Box* ui_box_make_f(const char* fmt, UI_Box_flags flags, ...);
void __ui_get_next_box_clay_element_config(UI_Box* box, Clay_ElementId clay_id, UI_Box_flags flags);

// - Box extension
void ui_extend_box_with_custom_draw_function(UI_Box* box, UI_Box_custom_draw_func_pointer_type* custom_draw, void* data);
void ui_extend_box_with_text(UI_Box* box, Str8 str);

// - Box data queries
UI_Box_data ui_box_data_from_id(Str8 id);
UI_Box_data ui_box_data_from_box(UI_Box* box);
UI_Actions ui_actions_from_box(UI_Box* box);
UI_Actions ui_actions_from_id(Str8 id);
UI_Actions ui_actions_from_id_f(const char* fmt, ...);
V2F32 ui_clip_offset_from_box(UI_Box* box);
V2F32 ui_get_prev_build_scroll_for_box(UI_Box* box);


// - Box setters // TODO: This is new, might not be used later
void ui_box_set_clip_offset_for_axis(UI_Box* box, F32 clip_offset, Axis2 axis);
void ui_box_set_clip_offset_x(UI_Box* box, F32 clip_offset);
void ui_box_set_clip_offset_y(UI_Box* box, F32 clip_offset);
void ui_box_set_clip_offset(UI_Box* box, V2F32 clip_offset);

// - Size makers // TODO: This is not where it is here in the .cpp file, fix this
UI_Size ui_size_make(UI_Size_kind kind, F32 value1, F32 value2);
UI_Size ui_px(F32 value);                 
UI_Size ui_rem(F32 scale);                 
UI_Size ui_fit_mm(F32 min, F32 max);      
UI_Size ui_grow_mm(F32 min, F32 max);     
UI_Size ui_fit();                         
UI_Size ui_grow();                        
UI_Size ui_p_of_p(F32 p);                 

// - Other
U64 ui_get_build_generation();
Arena* ui_get_build_arena();
V2F32 ui_get_mouse_pos();
V2F32 ui_get_prev_mouse_pos();
UI_Box* ui_get_current_parent();
UI_Box* ui_get_root(); // TODO: This might need a better name that specifies weather this is from the prev build or the new build
UI_Box* ui_find_box_in_tree_by_id(UI_Box* root, Str8 id);
UI_Box* ui_find_prev_build_box_by_id(Str8 id);
UI_Box* ui_find_prev_build_box_by_box(UI_Box* box);

// - Stack functions and helper
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DECLARE_PUSH_FUNC)
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DECLARE_POP_FUNC)
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DECLARE_AUTO_POP_FUNC)
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DECLARE_GET_FUNC)
//
V4F32 ui_top_padding();
V4F32 ui_top_corner_radius();
V4F32 ui_top_border_width();
//
void ui_next_width(UI_Size size);
void ui_next_height(UI_Size size);
// 
void ui_next_layout_x();
void ui_next_layout_y();

// - Box style setters for already created boxed
void ui_set_box_b_color(UI_Box* box, V4F32 color);


// TODO: There are some more there that you have defined and have not moved to the .h file yet

// - Macros for automatic stack pushing and popping
// Damian: I would like to do something like that, have a macro that generates macros, but that is not possible in c/cpp.
//         This is exactly the reason why ryan has his own table tool generation. Also his tool outputs files that you
//         then can go read the code for, which is the issue with macros - you dont really see the final code. 
//         So gonna have to define the DeferLoop macros for push and pops manually
// 
// #define __UI_STACK_DEFINE_DEFER_PUSH_POP_MACROS(Stack_type_name, inner_data_type, var_name_inside_state, default_expr, push_func_name, set_next_func_name, pop_func_name, auto_pop_func_name, get_top_func_name, stack_arr_capacity, defer_push_pop_macro_name) \
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
#define UI_AlignmentX(v)              DeferLoop(ui_push_alignment_x(v),                ui_pop_alignment_x())
#define UI_AlignmentY(v)              DeferLoop(ui_push_alignment_y(v),                ui_pop_alignment_y())
#define UI_HoverCursor(v)             DeferLoop(ui_push_hover_cursor(v),               ui_pop_hover_cursor())
//
#define UI_Width(v) UI_SizeX(v)
#define UI_Height(v) UI_SizeY(v)
#define UI_Padding(v) UI_PaddingLeft(v) UI_PaddingTop(v) UI_PaddingRight(v) UI_PaddingBottom(v)

///////////////////////////////////////////////////////////
// - Helpers to wrap around clay
//
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
Clay_CornerRadius __ui_clay_corner_radius_from_v2f32 (V4F32 vec);

#endif






