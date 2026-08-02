/*  date = July 09th 2026  */


/*
NOTE(S): Not using any of your abstractions yet, going super simple only using the base layer. When I have something I'm happy with, I'll integrate with your os layer.
TODO(S): Need to use same tab size, also I think my editor doesn't sub out spaces for tabs.
*/

#include "core/core_include.h"
#include "core/core_include.cpp"


#include <Windows.h>
#include "psapi.h"
#include "nt_types.h"

#define WIN32_MAX_WIDE_PATH 32768




struct VersionInfoTranslation
{
  WORD language;
  WORD codepage;
};

tu_specific Str8
DisplayNameFromPid(Arena* arena, DWORD pid)
{
  ProfBeginFunc();
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
  ProfEndGroup();
  return result;
};

// NOTE(S): Your function copy-pasted
tu_specific Str8 
ExeNameFromPid(Arena* arena, S32 pid)
{
  ProfBeginFunc();
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
  ProfEndGroup();
  return result_str;
}


typedef struct WindowInfo WindowInfo;
struct WindowInfo
{
  HWND hwnd;
  DWORD pid;
  DWORD ppid;
  Str8 exe_name;
  // Str8 title;  // TODO(S): GET WINDOW TITLE
  Str8 display_name;
};

typedef struct WindowInfoArray WindowInfoArray;
struct WindowInfoArray
{
  WindowInfo* v;
  U64 count;

  WindowInfo& operator[](U64 i) { Assert(i < count); return v[i]; }
}; 

#define CHUNK_BLOCK_SIZE 64

typedef struct HwndChunk HwndChunk;
struct HwndChunk
{
  HWND items[CHUNK_BLOCK_SIZE];
  HwndChunk* next;
  U32 count;
};

typedef struct EnumWindowsCtx EnumWindowsCtx;
struct EnumWindowsCtx
{
  Arena* scratch_arena;
  HwndChunk* first_chunk;
  HwndChunk* last_chunk;
  U64        total_count;
};

// TODO(S): Come up with more filters and use non-blocking functions
tu_specific BOOL CALLBACK 
Win32EnumWindowsCallback(HWND hwnd, LPARAM lparam)
{
  EnumWindowsCtx* ctx = (EnumWindowsCtx* )lparam;

  // NOTE(S): Filter windows that don't have WS_VISIBLE
  if (!IsWindowVisible(hwnd))
    return TRUE;

  // NOTE(S): Filter windows that are owned by another window.
  if (GetWindow(hwnd, GW_OWNER) != NULL)
    return TRUE;

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
  ProfBeginFunc();
  Temp_arena scratch  = get_scratch(&arena, 1);
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
  ProfEndGroup();
  return result;
}




// NOTE(S): if using functions that are only available in newer Windows versions, dynamically import function.
// And that's a big IF.
#pragma comment(lib, "ntdll") 

#define Str8Varg(S) (int)((S).count), ((S).data)     // use this for variadic functions where the format specifier is "%.*s" meaning an int value (width) is provided before the char string.
int main()
{
  // TODO(S): Support Unicode!
  allocate_thread_context();

  B32 os_init_succ = os_init();
  if (!os_init_succ) { return -1; }
  OS_State* win32_state = os_get_state();
  profiler_init();
  ProfBeginGroup("MAIN");
  
  Arena* frame_arena = arena_alloc(Gigabytes(64));
  int frame_count = 0;
  for (;frame_count <= 1;)
  {
    frame_count++;

    WindowInfoArray windows = GetTrackableWindows(frame_arena);   

    for (U64 i{}; i < windows.count; ++i)
    {
      WindowInfo* w = &windows[i];
      printf("hwnd=%p pid=%lu exe=%.*s display=%.*s\n",
              w->hwnd, w->pid,
              Str8Varg(w->exe_name),
              Str8Varg(w->display_name));
              // Str8Varg(w->title),
    }
    


    arena_clear(frame_arena);
    // TODO(S): Implement proper timer
    // Sleep(2000);
  }
  release_thread_context();
  ProfEndGroup();
  profiler_release();
  return 0;
  
}
