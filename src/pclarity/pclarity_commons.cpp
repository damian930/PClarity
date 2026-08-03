#ifndef PCLARITY_COMMONS_CPP
#define PCLARITY_COMMONS_CPP

#include "core/core_include.cpp"
#include "os/win32.cpp"
#include "font_provider/font_provider.cpp"
#include "render/render.cpp"
#include "draw/draw.cpp"
#include "ui/ui_core.cpp"

#include "pclarity/pclarity_commons.h"
#include "pclarity/win32_data_retrival/win32_data_retrival.cpp"

void pcl_defer_command_to_start_of_next_frame(PCL_State* PCL, PCL_Command command)
{
  PCL_Command_node* command_node = ArenaPush(PCL->frame_arena, PCL_Command_node);
  command_node->command = command;
  PCL_Command_list* command_list = &PCL->defered_commands_to_start_of_next_frame;
  DllPushBack(command_list, command_node);
  command_list->count += 1;
}

#endif