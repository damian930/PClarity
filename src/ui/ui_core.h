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

// // Controls the alignment along the y axis (vertical) of child elements.
// typedef CLAY_PACKED_ENUM {
//     // (Default) Aligns child elements to the top of this element, offset by padding.width.top
//     CLAY_ALIGN_Y_TOP,
//     // Aligns child elements to the bottom of this element, offset by padding.width.bottom
//     CLAY_ALIGN_Y_BOTTOM,
//     // Aligns child elements vertically to the center of this element
//     CLAY_ALIGN_Y_CENTER,
// } Clay_LayoutAlignmentY;

// TODO: Here will be all the box data that we need that will then be used in the clay thing
struct UI_Box {
  Clay_ElementDeclaration clay_element_config;
  B32 has_been_updated_this_frame;

  B32 has_hover_cursor;
  OS_Cursor hover_cursor;

  UI_Box* first_child;
  UI_Box* last_child;
  UI_Box* next_sibling;
  UI_Box* prev_sibling;
  UI_Box* parent;
  U64 children_count;
};

// TODO: Move this to a better place
// TODO: Also redo the __UI_NULL_BOX_VALUE since it might be wrong if the order of the box field have changed since you did the macor
#define __UI_NULL_BOX_VALUE { \
  {}, \
  {}, \
  {}, \
  {}, \
  &__ui_g_null_box, \
  &__ui_g_null_box, \
  &__ui_g_null_box, \
  &__ui_g_null_box, \
  &__ui_g_null_box, \
  {}, \
}
global UI_Box __ui_g_null_box = __UI_NULL_BOX_VALUE;

// This is separated into a separete file just cause its easier to have
// macros be there, i think.
// Some of those macros need some types from above, so we include it here.
#include "ui/ui_stack_macros.h" 

// Stack structs
__UI_STACK_DATA_TABLE_EXPANSION(__UI_STACK_DEFINE_STACK_STRUCTS)

struct UI_Box_id_node {
  Clay_ElementId id;
  UI_Box_id_node* next;
};

struct UI_Box_id_list {
  UI_Box_id_node* first;
  UI_Box_id_node* last;
  U64 count;
};

// TODO: Move data fiels in state for better structural meaning
struct UI_State {
  // TODO: Add a counter for boxes made last build
  // TODO: Add a build counter just for debug purposes if we need to

  Arena* state_arena;
  Arena* arena_for_clay; // This is clay internal memory

  Arena* build_arena;
  Clay_RenderCommandArray render_commands_as_result_of_ui_build;

  UI_Box_id_list hovered_ids;
  UI_Box* final_hover_box;

  U64 build_generation;

  struct {
    Clay_ElementId clay_id;
    B32 is_mouse_down;
    B32 did_mouse_leave_box_while_was_down;
  } interacted_with_box_data;

  UI_Box* root_box;
  V2F32 mouse_pos_for_this_build;
  V2F32 window_dims_for_this_build;

  // Locking them under a struct so ui_state is easier to view in the debugger
  struct {
    #define EXPANSION(Stack_type_name, inner_data_type, var_name_inside_state, ...) Stack_type_name var_name_inside_state;
    __UI_STACK_DATA_TABLE_EXPANSION(EXPANSION)
    #undef EXPANSTION
  } stacks;
};

// // - Context variables
struct UI_State;
extern UI_State* __ui_g_state;

// - State 
UI_State* ui_get_state();
void ui_set_state(UI_State* context);
void ui_init();
void ui_release();

// TODO: Move these out of here
// - Simple getters
// Arena* ui_get_build_arena();
// F32 ui_get_mouse_x();
// F32 ui_get_mouse_y();
// V2F32 ui_get_mouse_pos();

// // - IDs
// Str8 ui_get_text_part_from_str8(Str8 id_and_text);

// - UI building
void ui_begin_build(V2F32 window_dims, V2F32 mouse_pos, FP_Font default_font);
void ui_end_build();
// =========
// TODO: Move this somwhere if ends up beeeing used
UI_Box* __ui_find_box_by_id_helper(UI_Box* root, Clay_ElementId id);
UI_Box* ui_find_box_by_id(Clay_ElementId id);
// =========
void __ui_build_clay_element_tree_from_box_tree(UI_Box* root);
#define UI_Build(window_dims, mouse_pos) DeferLoop(ui_begin_build(window_dims, mouse_pos), ui_end_build())

// - UI drawing
void ui_draw();

// - Box making
B32 ui_box_is_null(UI_Box* box);
UI_Box* ui_null_box();
UI_Box* ui_box_make(Str8 id_and_text, UI_Box_flags flags);
UI_Box* ui_box_make_f(const char* fmt, UI_Box_flags flags, ...);
void __ui_get_next_box_clay_element_config(Clay_ElementDeclaration* config, Clay_ElementId clay_id, UI_Box_flags flags);

// - Box custom draw extention
struct UI_Provided_data_for_custom_draw {
  Rect final_box_rect;
  V4F32 background_color;
  V4F32 corner_radii;
};
#define UI_CUSTOM_DRAW_BOX_DEF(name) void name(UI_Provided_data_for_custom_draw provided_data, void* custom_data)
typedef UI_CUSTOM_DRAW_BOX_DEF(UI_Box_custom_draw_func_type);
void ui_extend_box_with_custom_draw_function(UI_Box* box, UI_Box_custom_draw_func_type* custom_draw, void* data);

// - Size makers
UI_Size ui_size_make(UI_Size_kind kind, F32 value1, F32 value2);
UI_Size ui_px(F32 value);                 
UI_Size ui_fit_mm(F32 min, F32 max);      
UI_Size ui_grow_mm(F32 min, F32 max);     
UI_Size ui_fit();                         
UI_Size ui_grow();                        
UI_Size ui_p_of_p(F32 p);                 

// - Other
Arena* ui_get_build_arena();

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
//
#define UI_Width(v) UI_SizeX(v)
#define UI_Height(v) UI_SizeY(v)
#define UI_Padding(v) UI_PaddingLeft(v) UI_PaddingTop(v) UI_PaddingRight(v) UI_PaddingBottom(v)

///////////////////////////////////////////////////////////
// - Helpers to wrap around clay
//
Clay_SizingAxis   __ui_clay_sizing_axis_from_ui_size (UI_Size ui_size);
Clay_Padding      __ui_clay_padding_from_v4f32       (V4F32 padding);
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

// // - Default box settings stacks

// void         ui_push_flags(UI_Box_flags v);       
// void         ui_pop_flags(); 
// void         ui_set_next_flags(UI_Box_flags v);       
// void         ui_pop_single_usage_flags();
// UI_Box_flags ui_get_flags();
// //
// void  ui_push_layout_axis(Axis2 v);       
// void  ui_pop_layout_axis(); 
// void  ui_set_next_layout_axis(Axis2 v);       
// void  ui_pop_single_usage_layout_axis();
// Axis2 ui_get_layout_axis();
// //
// void    ui_push_size_x(UI_Size v);          
// void    ui_pop_size_x();      
// void    ui_set_next_size_x(UI_Size v);          
// void    ui_pop_single_usage_size_x();
// UI_Size ui_get_size_x();
// //
// void    ui_push_size_y(UI_Size v);          
// void    ui_pop_size_y();      
// void    ui_set_next_size_y(UI_Size v);          
// void    ui_pop_single_usage_size_y();
// UI_Size ui_get_size_y();
// //
// void ui_push_border_width(F32 v);          
// void ui_pop_border_width();      
// void ui_set_next_border_width(F32 v);          
// void ui_pop_single_usage_border_width();
// F32  ui_get_border_width();
// //
// void  ui_push_border_color(V4F32 v);          
// void  ui_pop_border_color();      
// void  ui_set_next_border_color(V4F32 v);          
// void  ui_pop_single_usage_border_color();
// V4F32 ui_get_border_color();
// //
// void ui_push_padding(F32 v);          
// void ui_pop_padding();      
// void ui_set_next_padding(F32 v);          
// void ui_pop_single_usage_padding();
// F32  ui_get_padding();
// //
// void ui_push_child_gap(F32 v);          
// void ui_pop_child_gap();      
// void ui_set_next_child_gap(F32 v);          
// void ui_pop_single_usage_child_gap();
// F32  ui_get_child_gap();

// #define UI_LayoutAxis(axis2)  DeferLoop(ui_push_layout_axis(axis2),       ui_pop_layout_axis())
// #define UI_SizeX(ui_size)     DeferLoop(ui_push_semantic_size_x(ui_size), ui_pop_semantic_size_x())
// #define UI_SizeY(ui_size)     DeferLoop(ui_push_semantic_size_y(ui_size), ui_pop_semantic_size_y())
// // #define UI_Padding(padding)   DeferLoop(ui_push_padding(padding),         ui_pop_padding())

// // - Style box settings stacks
// void  ui_push_b_color_uv(UV uv, V4F32 v);     
// void  ui_pop_b_color_uv(UV uv);               
// void  ui_set_next_b_color_uv(UV uv, V4F32 v); 
// void  ui_pop_single_usage_b_color_uv(UV uv);
// V4F32 ui_get_b_color_uv(UV uv);               

// void ui_push_b_color(V4F32 v);
// void ui_pop_b_color();
// void ui_set_next_b_color(V4F32 v);
// void ui_pop_single_usage_b_color();

// void  ui_push_corner_r(V4F32 v);
// void  ui_pop_corner_r();
// void  ui_set_next_corner_r(V4F32 v);
// void  ui_pop_single_usage_corner_r();
// V4F32 ui_get_corner_r();

// void ui_push_softness(F32 softness);
// void ui_pop_softness();
// void ui_set_next_softness(F32 softness);
// void ui_pop_single_usage_softness();
// F32  ui_get_softness();

// #define UI_BColor(v)            DeferLoop(ui_push_b_color(v),           ui_pop_b_color())
// #define UI_Border(width, color) DeferLoop(ui_push_border(width, color), ui_pop_border())
// #define UI_CornerR(v)           DeferLoop(ui_push_corner_r(v), ui_pop_corner_r())
// #define UI_Softness(v)          DeferLoop(ui_push_softness(v), ui_pop_softness())

// // - Style stack operations for text
// // void ui_push_text_color(V4F32 v);
// // void ui_pop_text_color();
// // void ui_set_next_text_color(V4F32 v);
// // V4F32 ui_get_text_color();

// void    ui_push_font(FP_Font v);
// void    ui_pop_font();
// void    ui_set_next_font(FP_Font v);
// void    ui_pop_single_usage_font();
// FP_Font ui_get_font();

// #define UI_TextColor(color) DeferLoop(ui_push_text_color(color), ui_pop_text_color())
// #define UI_Font(font)       DeferLoop(ui_push_font(font),        ui_pop_font())

// ====================
// ====================
// ====================
// ====================
// ====================
/* List of things i think i have to be able to do with this ui for it to be ok --> 
    UI SYSTEM — COMPLEXITY LADDER
    ==============================

    TIER 1 — STATIC PRIMITIVES
    ---------------------------
    01. [x] - Text / Typography      Font scale, weight, color tokens. Headings, body, captions, code spans.
    02. [x] - Color Swatch           A box that is purely a color. The atom of your theme system.
    03. [x] - Divider                Horizontal/vertical rule. May carry a label.
    04. [x] - Spacer                 Invisible box that enforces spacing units.
    05. [x] - Icon                   SVG  glyph at a fixed size. Inherits color.
    06. [x] - Avatar                 Image or initials in a circle/square. Fixed sizes.
    07. [x] - Badge / Tag            Small pill with text and optional color variant.
    08. [x] - Spinner / Loader       Animated indicator of indeterminate progress.
    09. [ ] - Skeleton               Placeholder shape while content loads.
    10. [ ] - Image / Media Box      Constrained image with aspect ratio and object-fit.


    TIER 2 — INTERACTIVE ATOMS
    ---------------------------
    11. [x] - Button                 Primary, secondary, ghost, destructive. Disabled state. Icon slot.
    12. [ ] - Icon Button            Square button with only an icon. Needs tooltip.
    13. [ ] - Link                   Inline or standalone. Underline, hover, visited states.
    14. [x] - Checkbox               Checked, unchecked, indeterminate. Label slot.
    15. [x] - Radio                  Single selection from a group. Label slot.
    16. [ ] - Toggle / Switch        Binary on/off. Animated thumb.
    17. [ ] - Text Input             Single-line. Placeholder, label, helper, error states.
    18. [ ] - Textarea               Multi-line input. Auto-resize variant.
    19. [ ] - Select / Dropdown      Native or custom. Option list, placeholder, disabled.
    20. [x] - Slider                 Range input. Single handle, optional value tooltip.


    TIER 3 — STATEFUL COMPONENTS
    -----------------------------
    21. Tooltip                Appears on hover/focus. Positioned relative to trigger.
    22. Popover                Floating panel anchored to a trigger. Dismissable.
    23. Accordion              Expand/collapse a section. Animated height.
    24. Tabs                   Switch between panels. Active indicator. Keyboard nav.
    25. Progress Bar           Determinate fill. Value, label, color variants.
    26. Alert / Banner         Info, success, warning, error. Dismissable.
    27. Toast / Snackbar       Timed notification. Stacking, dismiss, action.
    28. Modal / Dialog         Overlay with focus trap. Header, body, footer.
    29. Drawer / Sheet         Slides in from edge. Top, right, bottom, left.
    30. Chip / Tag Input       Add and remove tags inline within an input.
    31. File Upload            Drop zone + file list. Progress per file.
    32. Color Picker           Hue/saturation canvas + hex input.


    TIER 4 — COMPOSITE PATTERNS
    ----------------------------
    33. Card                   Surface with header, body, footer, media slot and actions.
    34. List / List Item       Virtualisable list. Icon, text, meta, action per row.
    35. Menu / Context Menu    Triggered list of actions. Groups, separators, icons.
    36. Command Palette        Search-driven action launcher. Keyboard-first.
    37. Combobox / Autocomplete  Input + filterable dropdown. Multi-select variant.
    38. Date Picker            Calendar grid + input. Range selection variant.
    39. Breadcrumb             Hierarchical path nav. Collapse on overflow.
    40. Pagination             Page controls with prev/next and jump-to.
    41. Table                  Sort, filter, row selection, sticky columns/header.
    42. Tree View              Nested hierarchy. Expand/collapse, selection.
    43. Stepper / Wizard       Multi-step flow. Linear or branching progress.
    44. Notification Center    List of past notifications. Read/unread state.


    TIER 5 — FULL SURFACES
    -----------------------
    45. Navigation Bar         Top or side. Logo, links, actions, mobile hamburger.
    46. Sidebar / Nav Rail     Collapsible. Active state, nested groups, icons + labels.
    47. Data Grid              Editable cells, column resize, row grouping, virtual scroll.
    48. Kanban Board           Drag-and-drop columns and cards. Add/edit inline.
    49. Rich Text Editor       Toolbar + editable area. Formatting, links, embeds.
    50. Form Builder           Dynamic form with validation, field groups, submit.
    51. Dashboard Layout       Grid of resizable, draggable widget tiles.
    52. Chat / Message Feed    Bubbles, timestamps, reactions, scroll-to-bottom.
    53. Calendar View          Month/week/day grid. Event placement, drag to reschedule.
    54. Settings Page          Sectioned form. Sidebar nav, save state, confirmation.
*/

#endif






