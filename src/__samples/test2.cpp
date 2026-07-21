/*  date = July 09th 2026  */


/*
NOTE(S): Not using any of your abstractions yet, going super simple only using the base layer. When I have something I'm happy with, I'll integrate with your os layer.
TODO(S): Need to use same tab size, also I think my editor doesn't sub out spaces for tabs.
*/

#include "core/core_include.h"
#include "core/core_include.cpp"

#include <Windows.h>
#include "nt_types.h"

#define WIN32_MAX_WIDE_PATH 32768

struct ProcessInfoNode
{
  S32 pid;
  S32 ppid;
  Str8 create_time;
  Str8 image_name;
  Str8 display_name;

  ProcessInfoNode* next;
};
struct ProcessInfoList
{
  ProcessInfoNode* first;
  ProcessInfoNode* last;
  U64 count;
};
struct ProcessInfoArray
{
  ProcessInfoNode* v;
  U64 count;

  ProcessInfoNode& operator[](U64 i)        { Assert(i < count); return v[i]; }
};

Str8 Str8FromTime(Arena* arena, LARGE_INTEGER time)
{
  RtlSystemTimeToLocalTime(&time, &time);
  TIME_FIELDS tf;
  RtlTimeToTimeFields(&time, &tf);
  return str8_fmt(arena, 
        "%02d:%02d:%02d.%03d",
        tf.Hour, tf.Minute, tf.Second, tf.Milliseconds);
}

#if 0

Get()
{
  B32 category = IsExeOrPackagedApp();
  
}

#endif

struct VersionInfoTranslation
{
  WORD language;
  WORD codepage;
};

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
}

tu_specific Str8 
Str8FromUnicodeString(Arena* arena, UNICODE_STRING u_str)
{
  Str8 result{};

  result.count = WideCharToMultiByte(CP_UTF8, 0, (wchar_t *)u_str.Buffer, u_str.Length, nullptr, 0, nullptr, nullptr);
  if (result.count) 
  {
    result.data = ArenaPushArr(arena, U8, result.count);
    WideCharToMultiByte(CP_UTF8, 0, (wchar_t *)u_str.Buffer, u_str.Length, (LPSTR)result.data, (int)result.count, nullptr, nullptr);
  }
  return result;
}

tu_specific void
ProcessInfoListAppend(Arena* arena, ProcessInfoList* list, SYSTEM_PROCESS_INFORMATION* p)
{
  ProcessInfoNode* node = ArenaPush(arena, ProcessInfoNode);

  node->next          = nullptr;
  node->pid           = HandleToLong(p->UniqueProcessId);
  node->ppid          = HandleToLong(p->InheritedFromUniqueProcessId);
  node->create_time   = Str8FromTime(arena, p->CreateTime);
  node->display_name  = DisplayNameFromPid(arena, node->pid);
  node->image_name    = Str8FromUnicodeString(arena, p->ImageName);

  QueuePushBack_Ex(list, node, first, last, next, is_zero_pointer, 0);
  ++list->count;
}

tu_specific ProcessInfoArray
Win32QueryProcessArray(Arena* arena)
{
  Scratch scratch = get_scratch(&arena, 1);

  ProcessInfoArray result{};
  ProcessInfoList result_list{};
  ULONG size{};
  
  // Query buffer size
  NTSTATUS status = NtQuerySystemInformation(SystemProcessInformation, nullptr, 0, &size);
  U8* proc_info_array{};

  for (;status == STATUS_INFO_LENGTH_MISMATCH;)
  {

    Temp_arena temp = temp_arena_begin(scratch.arena);

    proc_info_array = ArenaPushArr(temp.arena, U8, size);

    // Use queried buffer size
    status = NtQuerySystemInformation(SystemProcessInformation, proc_info_array, size, &size); // NOTE(S): the last param &size, updates to the new size windows wanted

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
      temp_arena_end(&temp);
    }
  }

  if (!NT_SUCCESS(status))
  {
    printf("NtQuerySystemInformation failed");
    return {};
  }


  auto* p = (SYSTEM_PROCESS_INFORMATION*)proc_info_array;

  for (;;)
  {
    ProcessInfoListAppend(scratch.arena, &result_list, p);

    if (p->NextEntryOffset == 0) 
      break;
    p = (SYSTEM_PROCESS_INFORMATION*)((PBYTE)p + p->NextEntryOffset);
  }
  

  result.v = ArenaPushArr(arena, ProcessInfoNode, result_list.count);
  result.count = result_list.count;
  U32 i{};
  for (auto* node = result_list.first; 
       node != nullptr; 
       node = node->next, ++i)
  {
    MemCopyStruct(&result[i], node);
  }
  end_scratch(&scratch);



  return result;
}



// NOTE(S): if using functions that are only available in newer Windows versions, dynamically import function.
// And that's a big IF.
#pragma comment(lib, "ntdll") 

int main()
{
  // TODO(S): Support Unicode!
  allocate_thread_context();
  B32 os_init_succ = os_init();
  if (!os_init_succ) { return -1; }
  OS_State* win32_state = os_get_state();
  
  Arena* frame_arena = arena_alloc(Gigabytes(64));
  for (;!os_window_should_close();)
  {
    ProcessInfoArray p = Win32QueryProcessArray(frame_arena); // <-- USE THIS FUNCTION
    
    #define Str8Varg(S) (int)((S).count), ((S).data)     // use this for variadic functions where the format specifier is "%.*s" meaning an int value (width) is provided before the char string.


    for (int i{}; i < p.count; ++i)
    {
      printf("Display Name: %.*s Image name: %.*s\n",
            Str8Varg(p[i].display_name), Str8Varg(p[i].image_name));
    }


    arena_clear(frame_arena);
    // TODO(S): Implement proper timer
    Sleep(2000);
  }

  return 0;
  
}

