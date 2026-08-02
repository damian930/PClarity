#ifndef WIN32_DATA_RETRIVAL_CPP
#define WIN32_DATA_RETRIVAL_CPP

#include "pclarity/win32_data_retrival/win32_data_retrival.h"

#include "core/core_include.cpp" 

// TODO(S): Come up with more filters and use non-blocking functions
tu_specific BOOL CALLBACK 
Win32EnumWindowsCallback(HWND hwnd, LPARAM lparam)
{
  EnumWindowsCtx* ctx = (EnumWindowsCtx* )lparam;

  // NOTE(S): Filter windows that don't have WS_VISIBLE
  // TODO, DD: COmment this back out 
  // if (!IsWindowVisible(hwnd))
    // return TRUE;

  // NOTE(S): Filter windows that are owned by another window.
  // TODO, DD: COmment this back out 
  // if (GetWindow(hwnd, GW_OWNER) != NULL)
    // return TRUE;

  // NOTE(S): Filter windows that don't have the force on taskbar and show up in Alt+Tab.
  // WS_EX_APPWINDOW forces on to task bar and Alt+Tab 
  // WS_EX_TOOLWINDOW means to hide it from task bar and ALt+Tab
  LONG ex_style = GetWindowLong(hwnd, GWL_EXSTYLE);
  if ((ex_style & WS_EX_TOOLWINDOW) && !(ex_style & WS_EX_APPWINDOW))
    return TRUE;

  S32 length = GetWindowTextLength(hwnd);
  if (length == 0)
    return TRUE;

  // NOTE(S): Filter windows visible with WS_VISIBLE but hidden by compositor
  // compositor can hide for a number of reasons (most I don't know)
  BOOL cloaked = FALSE;
  DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
  if (cloaked)
    return TRUE;

  // filters passed
  if (!ctx->last_chunk || ctx->last_chunk->count >= CHUNK_BLOCK_SIZE)
  {
    HwndChunk* chunk = ArenaPushArr(ctx->scratch_arena, HwndChunk, 1);
    QueuePushBack_Ex(ctx, chunk, first_chunk, last_chunk, next, is_zero_pointer, 0);
  }

  ctx->last_chunk->items[ctx->last_chunk->count++] = hwnd;
  ctx->total_count++;

  return TRUE;

}

tu_specific WindowInfoArray
GetTrackableWindows(Arena* arena)
{
  Temp_arena scratch = get_scratch(&arena, 1);
  WindowInfoArray result{};
  EnumWindowsCtx ctx{};
  ctx.scratch_arena = scratch.arena;
  
  // TODO(S): See how to enumerate UWP apps too
  if (EnumWindows(Win32EnumWindowsCallback, (LPARAM)&ctx))  
  {
    result.v = ArenaPushArr(arena, WindowInfo, ctx.total_count);

    for (auto* chunk = ctx.first_chunk; chunk; chunk = chunk->next)
    {
      for (U64 i{}; i < chunk->count; ++i)
      {
        HWND hwnd = chunk->items[i];

        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (!pid)
          continue;

        Str8 exe_name     = ExeNameFromPid(arena, pid);
        Str8 display_name = DisplayNameFromPid(arena, pid);

        if (!exe_name.count)
          continue;

        WindowInfo* info = &result.v[result.count];
        info->hwnd          = hwnd;
        info->pid           = pid;
        info->exe_name      = exe_name;
        info->display_name  = display_name;
        result.count++;
        // info->title TODO(S): GET WINDOW TITLE
      }
    }
  }


  end_scratch(&scratch);
  return result;
}

tu_specific Str8
DisplayNameFromPid(Arena* arena, DWORD pid)
{
  Temp_arena scratch = get_scratch(&arena, 1);

  Str8 result{};
  wchar_t name[WIN32_MAX_WIDE_PATH];
  DWORD size = WIN32_MAX_WIDE_PATH;

  UINT translation_size{};
  VersionInfoTranslation* translations{};

  HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)pid);
  if (process)
  {
    if (QueryFullProcessImageNameW(process, 0, name, &size))
    {
      DWORD version_info_size = GetFileVersionInfoSizeW(name, nullptr);

      Data_buffer version_info = data_buffer_make(scratch.arena, version_info_size);

      if (version_info_size)
      {
        if (GetFileVersionInfoW(name, 0, version_info_size, version_info.data))
        {

          // Read list of languages and code pages
          if (VerQueryValueW(version_info.data, L"\\VarFileInfo\\Translation", (void**)&translations, &translation_size))  // does count null terminator from what I checked
          {
            // Read the file description for each language and code page
            U32 translation_count = translation_size / sizeof(VersionInfoTranslation);
            wchar_t sub_block[256];           // hopefully no one is crazy enough with big FileDescriptions

            for (U32 i{}; i < translation_count; ++i)
            {
              swprintf(sub_block, 256, L"\\StringFileInfo\\%04x%04x\\FileDescription", 
                      translations[i].language,
                      translations[i].codepage);

              UINT char_count{};
              void* file_description; // wchar_t*

              if (VerQueryValueW(version_info.data, sub_block, &file_description, &char_count))
              {
                result.count = WideCharToMultiByte(CP_UTF8, 0, 
                      (wchar_t *)file_description, char_count, 
                      nullptr, 0, 
                      nullptr, nullptr);
                if (result.count) 
                {
                  result.data = ArenaPushArr(arena, U8, result.count);

                  WideCharToMultiByte(CP_UTF8, 0, 
                                    (wchar_t *)file_description, char_count, 
                                    (LPSTR)result.data, (int)result.count, 
                                    nullptr, nullptr);
                }   

                // Stopping at first description found
                break;     
              }
            }
          }
        }
      }
    }
  }

  end_scratch(&scratch);

  return result;
};

// NOTE(S): Your function copy-pasted
tu_specific Str8 
ExeNameFromPid(Arena* arena, S32 pid)
{
  Str8 result_str = {};
  
  HANDLE process_handle = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, (U32)pid);
  if (process_handle) ScratchLoop(scratch, &arena, 1)
  {
    Data_buffer buffer = data_buffer_make(scratch.arena, 255);
    DWORD res = GetModuleBaseNameA(process_handle, Null, (char*)buffer.data, (U32)buffer.count);
    if (res != 0)
    {
      result_str = str8_copy(arena, str8_substring(buffer, 0, res));
    }
  }
  CloseHandle(process_handle);
  return result_str;
}


#endif
