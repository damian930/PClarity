#ifndef PCLARITY_COMMONS_H
#define PCLARITY_COMMONS_H

#include "core/core_include.h"
#include "core/core_include.cpp"

#include "os/win32.h"
#include "os/win32.cpp"

#include "render/render.h"
#include "render/render.cpp"

#include "draw/draw.h"
#include "draw/draw.cpp"

#include "font_provider/font_provider.h"
#include "font_provider/font_provider.cpp"

#include "ui/ui_core.h"
#include "ui/ui_core.cpp"

#include "ui/widgets/ui_widgets.h"
#include "ui/widgets/ui_widgets.cpp"

#include "pclarity/win32_data_retrival/win32_data_retrival.h"

enum PCL_Menu {
  PCL_Menu__home,
  PCL_Menu__settings,
};

enum PCL_Table_header_kind : U32 {
  PCL_Table_header_kind__NONE, 
  PCL_Table_header_kind__name, 
  PCL_Table_header_kind__pid, 
  PCL_Table_header_kind__ppid,
  PCL_Table_header_kind__startup_time,
};

struct PCL_Table_header {
  PCL_Table_header_kind kind;
  F32 flex_value;
  B32 is_used_for_sorting;
  B32 sort_small_to_big;
};

enum PCL_Command {
  PCL_Command__NONE,
  PCL_Command__go_to_settings,
  PCL_Command__go_to_home,
  PCL_Command__close_the_app, // TODO:
  PCL_Command__new_main_font_size,
  PCL_Command__add_header_to_table,
  PCL_Command__remove_header_from_table,
  PCL_Command__clear_table,
  PCL_Command__sort_by_header,
  PCL_Command__select_row,
  PCL_Command__COUNT,
};

struct PCL_Command_node {
  PCL_Command command;
  PCL_Command_node* next;
  PCL_Command_node* prev;
};

struct PCL_Command_list {
  PCL_Command_node* first;
  PCL_Command_node* last;
  U64 count;
};

enum PCL_Color_name : U32 {
  PCL_Color_name__NONE,
  PCL_Color_name__main_background,
  PCL_Color_name__secondary_background,
  PCL_Color_name__item_selected,
  PCL_Color_name__change_main_font_size,
  PCL_Color_name__COUNT,
};

#define PCL_TABLE_HEADER_MAX_COUNT 64
struct PCL_State {
  B32 is_command_window_open;
  PCL_Menu current_menu;

  B32 close_the_app;

  B32 show_debug_data;

  // Frame data
  Arena* frame_arena;
  PCL_Command_list defered_commands_to_start_of_next_frame;
  WindowInfoArray gathered_process_data_this_frame;
  
  // TODO: This maybe should not be here, not sure, but here cause i just needed it somewhere
  // Table data
  // TODO: This is old code
  // F32 table_header_flex_values[64];
  // U64 table_header_flex_value_count;

  struct {
    B32 is_selected;
    S32 pid; // DD: Right now this is used as a key
  } selected_row_data;

  // Config like data
  F32 main_font_size;

  // Data that is sometimes used along with commands
  struct {
    F32 new_font_size;
    PCL_Table_header_kind table_header_kind_for_new_table_header;
    U64 header_index_to_remove;
    U64 sort_by_header__header_index;
    S32 process_at_row_to_select_pid;
  } data_for_commands;

  struct {
    PCL_Table_header headers[PCL_TABLE_HEADER_MAX_COUNT];
    U64 header_count;
  } table_data;

  // Damian: Test ui state for context menu
  struct {
    B32 is_open;
    V2F32 offset;
    Str8 id;
  } context_menu_state;

  struct {
    R_Handle settings;
    R_Handle home;
    R_Handle magnifying_glass;
    R_Handle arrow_up;
    R_Handle arrow_down;
  } icons;

  F32 font_size_for_ui;

  // TODO: THe way this gets inited is not great
  V4F32 color_values_for_names[PCL_Color_name__COUNT];

  // TODO: THe way this gets inited is not great
  Str8 command_names_for_user[PCL_Command__COUNT];
};

// - UI commands for logic update
void pcl_defer_command_to_start_of_next_frame(PCL_State* PCL, PCL_Command command);

#endif