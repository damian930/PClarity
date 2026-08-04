#ifndef PCLARITY_CPP
#define PCLARITY_CPP

#include "core/core_include.cpp"
#include "os/win32.cpp"
#include "render/render.cpp"
#include "draw/draw.cpp"
#include "font_provider/font_provider.cpp"
#include "ui/ui_core.cpp"
#include "ui/widgets/ui_widgets.cpp"

#include "pclarity/pclarity.h"
#include "pclarity/ui_code_for_dll/pclarity_ui_dll_code.h"
//
#include "pclarity/win32_data_retrival/win32_data_retrival.cpp"

// TODO: Move thi to the bottom of this file
// TODO: THis is not the best in terms of handling when the passed in data is invalide or we dont have a header that is selected
U64 pcl_get_header_index_with_generation(PCL_State* pcl)
{
  if (pcl->selected_header_generation == 0) { BreakPoint("Handle this better, have like a null value then like in ui for boxes"); return 0; }

  U64 result_index = 0;
  for EachIndex(i, pcl->table_data.header_count)
  {
    if (pcl->table_data.headers[i].generation == pcl->selected_header_generation)
    {
      result_index = i;
      break;
    }
  }
  return result_index;
}

///////////////////////////////////////////////////////////
// - Main passes
//
PCL_State pcl_init()
{
  PCL_State state = {};

  state.current_menu   = PCL_Menu__home;
  state.main_font_size = 20;
  state.frame_arena = arena_alloc(Megabytes(64));

  state.icons.settings         = r_load_texture_from_file(Str8FromC("../data/icons/gear.png"));
  state.icons.home             = r_load_texture_from_file(Str8FromC("../data/icons/house.png"));
  state.icons.magnifying_glass = r_load_texture_from_file(Str8FromC("../data/icons/magnifying_glass.png"));
  state.icons.arrow_up         = r_load_texture_from_file(Str8FromC("../data/icons/arrow_up.png"));
  state.icons.arrow_down       = r_load_texture_from_file(Str8FromC("../data/icons/arrow_down.png"));

  state.font_size_for_ui = 24;

  state.color_values_for_names[0] = rgba_from_hex(0x00000000);
  state.color_values_for_names[1] = rgba_from_hex(0x191432FF);
  state.color_values_for_names[2] = rgba_from_hex(0x362753FF);
  state.color_values_for_names[3] = rgba_from_hex(0x774ac9FF);

  state.command_names_for_user[0] = Str8FromC("PCL_Command__NONE");
  state.command_names_for_user[1] = Str8FromC("Open Settings");
  state.command_names_for_user[2] = Str8FromC("Open Home");
  state.command_names_for_user[3] = Str8FromC("Exit");

  return state;
}

void pcl_frame_update(PCL_State* pcl)
{
  ProfBeginFunc();
 
  // TODO: Make sure that there are no headers inside the table with the same generation, might be a good idea to call it id and not generation for better meaning 

  // DD: Handling the defered commands 
  for (
    PCL_Command_node* command_node = pcl->defered_commands_to_start_of_next_frame.first; 
    command_node != 0;
    command_node = command_node->next  
  ) {
    switch (command_node->command)
    {
      case PCL_Command__COUNT: 
      default: { InvalidCodePath(); } break;
  
      case PCL_Command__NONE: {} break;  
      
      case PCL_Command__close_the_app: 
      { 
        pcl->close_the_app = true;
      } break;

      case PCL_Command__go_to_settings: 
      { 
        pcl->current_menu = PCL_Menu__settings;
      } break;

      case PCL_Command__go_to_home:
      {
        pcl->current_menu = PCL_Menu__home;
      } break;
  
      case PCL_Command__new_main_font_size:
      {
        pcl->main_font_size = pcl->data_for_commands.new_font_size;
      } break;

      // case PCL_Command__add_header_to_table:
      // {
      //   PCL_Table_header_kind kind = pcl->data_for_commands.table_header_kind_for_new_table_header;
      //   pcl_add_header_into_table(pcl, kind, 1); 
      // } break;

      case PCL_Command__add_EMPTY_header_as_last_header_or_right_after_selected_header:
      case PCL_Command__add_PID_header_as_last_header_or_right_after_selected_header:
      case PCL_Command__add_PPID_header_as_last_header_or_right_after_selected_header:
      case PCL_Command__add_Name_header_as_last_header_or_right_after_selected_header:
      case PCL_Command__add_Icon_header_as_last_header_or_right_after_selected_header:
      {
        PCL_Table_header_kind header_kind_to_add = PCL_Table_header_kind__NONE;
        if (0) {}
        else if (command_node->command == PCL_Command__add_EMPTY_header_as_last_header_or_right_after_selected_header) { header_kind_to_add = PCL_Table_header_kind__NONE; }
        else if (command_node->command == PCL_Command__add_PID_header_as_last_header_or_right_after_selected_header) { header_kind_to_add = PCL_Table_header_kind__pid; }
        else if (command_node->command == PCL_Command__add_PPID_header_as_last_header_or_right_after_selected_header) { header_kind_to_add = PCL_Table_header_kind__ppid; }
        else if (command_node->command == PCL_Command__add_Name_header_as_last_header_or_right_after_selected_header) { header_kind_to_add = PCL_Table_header_kind__name; }
        else if (command_node->command == PCL_Command__add_Icon_header_as_last_header_or_right_after_selected_header) { header_kind_to_add = PCL_Table_header_kind__icon; }

        // TODO: This might be a call to be honest
        B32 is_header_selected = (pcl->selected_header_generation != 0);
        if (is_header_selected) { 
          U64 header_index_to_add_after = pcl_get_header_index_with_generation(pcl);
          pcl_add_header_after_index_of_table(pcl, header_kind_to_add, 2, header_index_to_add_after); 
        } else {
          pcl_add_header_to_the_end_of_table(pcl, header_kind_to_add, 2);
        }
      } break;

      case PCL_Command__select_header:
      {
        // TODO: Maybe there should be some warning for the case when we fuck up on our end which should not happend but then what if it does, maybe some logging here or some like that
        U64 test_generation = pcl->data_for_commands.generation_of_header_to_select;
        for EachIndex(i, pcl->table_data.header_count)
        {
          if (pcl->table_data.headers[i].generation == test_generation)
          {
            pcl->selected_header_generation = test_generation;
            break;
          }
        }
      } break;

      // case PCL_Command__remove_header_from_table:
      // {
      //   U64 index = pcl->data_for_commands.header_index_to_remove;
      //   pcl->data_for_commands.header_index_to_remove = 0;

      //   U64 new_count = ArrShiftLeftFromIndex(pcl->table_data.headers, pcl->table_data.header_count, index);
      //   pcl->table_data.header_count = new_count;
      //   pcl->table_data.headers[pcl->table_data.header_count] = {};
      // } break;

      case PCL_Command__clear_table:
      {
        pcl->table_data = {};
      } break;

    }
  }

  // DD: Clearing per frame data before we start the new frame for real
  pcl->defered_commands_to_start_of_next_frame = {};
  pcl->data_for_commands                       = {};
  arena_clear(pcl->frame_arena);

  #if 0
  ProfGroup("Win32QueryProcessArray")
  {
    WindowInfoArray info_arr = GetTrackableWindows(pcl->frame_arena);
    pcl->gathered_process_data_this_frame = info_arr;
  }
  #else
  { // Making fake data since the data that we have from GetTrackableWindows sucks right now
    Arena* arena = pcl->frame_arena;

    struct window_info_fake_struct {
      HWND hwnd;
      DWORD pid;
      DWORD ppid;
      const char* exe_name;
    };

    window_info_fake_struct fake_data_arr[] = {
      {(HWND)0x100001, 4120, 1024, "explorer.exe"},
      {(HWND)0x100002, 4121, 1024, "chrome.exe"},
      {(HWND)0x100003, 4122, 1024, "firefox.exe"},
      {(HWND)0x100004, 4123, 1024, "notepad.exe"},
      {(HWND)0x100005, 4124, 1024, "cmd.exe"},
      {(HWND)0x100006, 4125, 1024, "powershell.exe"},
      {(HWND)0x100007, 4126, 1024, "Code.exe"},
      {(HWND)0x100008, 4127, 1024, "Discord.exe"},
      {(HWND)0x100009, 4128, 1024, "Spotify.exe"},
      {(HWND)0x10000A, 4129, 1024, "Steam.exe"},
      {(HWND)0x10000B, 4130, 1024, "Taskmgr.exe"},
      {(HWND)0x10000C, 4131, 1024, "calc.exe"},
      {(HWND)0x10000D, 4132, 1024, "mstsc.exe"},
      {(HWND)0x10000E, 4133, 1024, "msedge.exe"},
      {(HWND)0x10000F, 4134, 1024, "Teams.exe"},
      {(HWND)0x100010, 4135, 1024, "Slack.exe"},
      {(HWND)0x100011, 4136, 1024, "devenv.exe"},
      {(HWND)0x100012, 4137, 1024, "WINWORD.EXE"},
      {(HWND)0x100013, 4138, 1024, "EXCEL.EXE"},
      {(HWND)0x100014, 4139, 1024, "POWERPNT.EXE"},
      {(HWND)0x100015, 4140, 1024, "OUTLOOK.EXE"},
      {(HWND)0x100016, 4141, 1024, "OneDrive.exe"},
      {(HWND)0x100017, 4142, 1024, "Telegram.exe"},
      {(HWND)0x100018, 4143, 1024, "WhatsApp.exe"},
      {(HWND)0x100019, 4144, 1024, "GitHubDesktop.exe"},
      {(HWND)0x10001A, 4145, 1024, "GitKraken.exe"},
      {(HWND)0x10001B, 4146, 1024, "obs64.exe"},
      {(HWND)0x10001C, 4147, 1024, "vlc.exe"},
      {(HWND)0x10001D, 4148, 1024, "7zFM.exe"},
      {(HWND)0x10001E, 4149, 1024, "PaintDotNet.exe"},
      {(HWND)0x10001F, 4150, 1024, "mspaint.exe"},
      {(HWND)0x100020, 4151, 1024, "SnippingTool.exe"},
      {(HWND)0x100021, 4152, 1024, "regedit.exe"},
      {(HWND)0x100022, 4153, 1024, "services.exe"},
      {(HWND)0x100023, 4154, 1024, "mmc.exe"},
      {(HWND)0x100024, 4155, 1024, "control.exe"},
      {(HWND)0x100025, 4156, 1024, "Wireshark.exe"},
      {(HWND)0x100026, 4157, 1024, "Procmon.exe"},
      {(HWND)0x100027, 4158, 1024, "ProcessHacker.exe"},
      {(HWND)0x100028, 4159, 1024, "IDA64.exe"},
      {(HWND)0x100029, 4160, 1024, "x64dbg.exe"},
      {(HWND)0x10002A, 4161, 1024, "dnSpy.exe"},
      {(HWND)0x10002B, 4162, 1024, "FileZilla.exe"},
      {(HWND)0x10002C, 4163, 1024, "putty.exe"},
      {(HWND)0x10002D, 4164, 1024, "WinSCP.exe"},
      {(HWND)0x10002E, 4165, 1024, "Rider64.exe"},
      {(HWND)0x10002F, 4166, 1024, "pycharm64.exe"},
      {(HWND)0x100030, 4167, 1024, "javaw.exe"},
      {(HWND)0x100031, 4168, 1024, "node.exe"},
      {(HWND)0x100032, 4169, 1024, "python.exe"}
    };

    WindowInfoArray info_arr = {};
    info_arr.v     = ArenaPushArr(pcl->frame_arena, WindowInfo, ArrayCount(fake_data_arr));
    info_arr.count = ArrayCount(fake_data_arr);

    for EachIndex(i, ArrayCount(fake_data_arr))
    {
      window_info_fake_struct* fake_data = &fake_data_arr[i];
      WindowInfo* info                   = &info_arr.v[i];
      info->hwnd         = fake_data->hwnd;
      info->pid          = fake_data->pid;
      info->ppid         = fake_data->ppid;
      info->exe_name     = str8_from_cstr_view((U8*)fake_data->exe_name);
      info->display_name = Str8{};
    }

    pcl->gathered_process_data_this_frame = info_arr;
  }
  #endif

  for (OS_Event* ev = os_get_frame_event_list()->first; ev; ev = ev->next)
  {
    if (ev->kind == OS_Event_kind__key && ev->key_event.key == Key__f1 && ev->key_event.went_down)
    {
      pcl->show_debug_data = ToggleBool(pcl->show_debug_data);
      os_consume_frame_event(ev);
      break;
    }
  }

  // Todo: Here you should order the thing about the process data once you start using arrays for Win32QueryProcessList
  
  ProfEndGroup();
}
 
void pcl_add_header_to_the_end_of_table(PCL_State* pcl, PCL_Table_header_kind header_kind, F32 flex_value)
{
  if (pcl->table_data.header_count >= PCL_TABLE_HEADER_MAX_COUNT) { return; }
  
  pcl->table_header_generation_counter += 1;

  PCL_Table_header* new_header = pcl->table_data.headers + (pcl->table_data.header_count++);
  new_header->kind       = header_kind;
  new_header->flex_value = flex_value;
  new_header->generation = pcl->table_header_generation_counter;
}

void pcl_add_header_after_index_of_table(PCL_State* pcl, PCL_Table_header_kind header_kind, F32 flex_value, U64 index_to_add_after)
{
  // TODO: Test if the bound of the array for the table header dont overflow with this shitty ass code here

  if (pcl->table_data.header_count >= PCL_TABLE_HEADER_MAX_COUNT) { return; }
  index_to_add_after = clamp_u64(index_to_add_after, 0, pcl->table_data.header_count);

  // TODO: DOnt use the byte memmove here, use some macro
  U64 n_header_we_have_to_shift = pcl->table_data.header_count - 1 - index_to_add_after;
  if (n_header_we_have_to_shift > 0) {
    memmove(pcl->table_data.headers + index_to_add_after + 2, pcl->table_data.headers + index_to_add_after + 1, n_header_we_have_to_shift * sizeof(PCL_Table_header));
  }
  pcl->table_data.header_count += 1;

  pcl->table_header_generation_counter += 1;

  PCL_Table_header* new_header = &pcl->table_data.headers[index_to_add_after + 1];
  new_header->kind       = header_kind;
  new_header->flex_value = flex_value;
  new_header->generation = pcl->table_header_generation_counter;
}















#endif