/*  date = July 09th 2026  */

#ifndef WIN32_DATA_RETRIVAL_H
#define WIN32_DATA_RETRIVAL_H

#include "psapi.h"  // you can move this

#include "core/core_include.h" 

#define WIN32_MAX_WIDE_PATH 32768

// TODO, DD: dont use the win32 specic types here like DWORD And all those, at lest those should not be 
// on the outside of the api for the called but are fine to use for the internal things
typedef struct WindowInfo WindowInfo;
struct WindowInfo
{
  HWND hwnd;
  DWORD pid;
  DWORD ppid;
  Str8 exe_name;
  Str8 display_name;
  // Str8 title;  // TODO(S): GET WINDOW TITLE
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
Win32EnumWindowsCallback(HWND hwnd, LPARAM lparam);

tu_specific WindowInfoArray
GetTrackableWindows(Arena* arena);

/////////////////////////////////////////
//- Helpers

struct VersionInfoTranslation
{
  WORD language;
  WORD codepage;
};

tu_specific Str8
DisplayNameFromPid(Arena* arena, DWORD pid);

// NOTE(S): Your function copy-pasted
tu_specific Str8 
ExeNameFromPid(Arena* arena, S32 pid);

// TODO, DD: Remove this from here
#define Str8Varg(S) Str8FmtArg(S)

#if 0 // NOTE(S): Usage code here
int main()
{
  // TODO(S): Support Unicode!
  allocate_thread_context();

  B32 os_init_succ = os_init();
  if (!os_init_succ) { return -1; }
  OS_State* win32_state = os_get_state();
  profiler_init();
  
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
    Sleep(2000);
  }
  release_thread_context();
  profiler_release();

  return 0;
  
}
#endif

#endif WIN32_DATA_RETRIVAL_H