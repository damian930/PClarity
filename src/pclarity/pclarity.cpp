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

// TODO: This might not be used thought
U64 arr_shift_left_from_index(void* arr, U64 arr_count, U64 index_to_remove, U64 size_of_arr_entry)
{
  U64 new_count = arr_count;
  if (new_count > 0 && index_to_remove < new_count)
  {
    if (new_count > 1)
    {
      for (U64 i = index_to_remove; i < new_count - 1; i += 1)
      {
        U8* arr_entry      = ((U8*)(arr)) + ((i + 0) * size_of_arr_entry);
        U8* arr_next_entry = ((U8*)(arr)) + ((i + 1) * size_of_arr_entry);
        memcpy(arr_entry, arr_next_entry, size_of_arr_entry);
      }
    }
    new_count -= 1;
  }
  else { InvalidCodePath(); }
  return new_count;
}
#define ArrShiftLeftFromIndex(arr_p, arr_count, index_to_remove) arr_shift_left_from_index(arr_p, arr_count, index_to_remove, sizeof(arr_p[0]))

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

      case PCL_Command__add_header_to_table:
      {
        PCL_Table_header_kind kind = pcl->data_for_commands.table_header_kind_for_new_table_header;
        pcl_add_header_into_table(pcl, kind, 1); 
      } break;

      case PCL_Command__remove_header_from_table:
      {
        U64 index = pcl->data_for_commands.header_index_to_remove;
        pcl->data_for_commands.header_index_to_remove = 0;

        U64 new_count = ArrShiftLeftFromIndex(pcl->table_data.headers, pcl->table_data.header_count, index);
        pcl->table_data.header_count = new_count;
        pcl->table_data.headers[pcl->table_data.header_count] = {};
      } break;

      case PCL_Command__clear_table:
      {
        pcl->table_data = {};
      } break;
      
      case PCL_Command__sort_by_header:
      { 
        U64 chosen_header_index = pcl->data_for_commands.sort_by_header__header_index;
        pcl->data_for_commands.sort_by_header__header_index = 0;
        
        for EachIndex(header_index, pcl->table_data.header_count)
        {
          PCL_Table_header* header = pcl->table_data.headers + header_index;
          if (header_index == chosen_header_index)
          {
            if (header->is_used_for_sorting) { 
              header->sort_small_to_big = ToggleBool(header->sort_small_to_big);
            }
            else {
              header->is_used_for_sorting = true;
            }
          }
          else 
          {
            header->is_used_for_sorting = false;
            header->sort_small_to_big = false;
          }
        }
      } break;

      case PCL_Command__select_row:
      {
        pcl->selected_row_data.is_selected = true;
        pcl->selected_row_data.pid         = pcl->data_for_commands.process_at_row_to_select_pid;

        pcl->data_for_commands.process_at_row_to_select_pid = 0;
      } break;

    }
  }

  // DD: Clearing per frame data before we start the new frame for real
  pcl->defered_commands_to_start_of_next_frame = {};
  pcl->data_for_commands                       = {};
  arena_clear(pcl->frame_arena);

  // DD: Checking the state invariant
  B32 is_state_valid = true;
  {
    U64 number_of_sorting_header = 0;
    for EachIndex(header_index, PCL_TABLE_HEADER_MAX_COUNT)
    {
      if (pcl->table_data.headers[header_index].is_used_for_sorting) {
        number_of_sorting_header += 1;
      }
      if (number_of_sorting_header > 1) { 
        is_state_valid = false;
        break; 
      }
    }
  }
  Handle(is_state_valid);

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
 
void pcl_add_header_into_table(PCL_State* pcl, PCL_Table_header_kind header_kind, F32 flex_value)
{
  if (pcl->table_data.header_count >= PCL_TABLE_HEADER_MAX_COUNT) { return; }

  PCL_Table_header* new_header = pcl->table_data.headers + (pcl->table_data.header_count++);
  new_header->kind                    = header_kind;
  new_header->flex_value              = flex_value;
  new_header->is_used_for_sorting = false;
}















#endif