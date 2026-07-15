/*  date = July 09th 2026  */


/*
NOTE(S): Not using any of your abstractions yet, going super simple only using the base layer. When I have something I'm happy with, I'll integrate with your os layer.
TODO(S): Need to use same tab size, also I think my editor doesn't sub out spaces for tabs.
*/

#include "core/core_include.h"
#include "core/core_include.cpp"

#include <Windows.h>
#include "nt_types.h"

struct ProcessInfoNode
{
  S32 pid;
  S32 ppid;
  Str8 create_time;

  ProcessInfoNode* next;
};
struct ProcessInfoList
{
  ProcessInfoNode* first;
  ProcessInfoNode* last;
  U64 count;
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

tu_specific void
ProcessInfoListAppend(Arena* arena, ProcessInfoList* list, SYSTEM_PROCESS_INFORMATION* p)
{
  ProcessInfoNode* node = ArenaPush(arena, ProcessInfoNode);

  node->pid         = HandleToLong(p->UniqueProcessId);
  node->ppid        = HandleToLong(p->InheritedFromUniqueProcessId);
  node->create_time = Str8FromTime(arena, p->CreateTime);
  node->next        = nullptr;

  QueuePushBack_Name(list, node, first, last, next);
  ++list->count;
}

tu_specific ProcessInfoList
Win32QueryProcessList(Arena* arena)
{
  Scratch scratch = get_scratch(&arena, 1);

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
    ProcessInfoListAppend(arena, &result_list, p);

    if (p->NextEntryOffset == 0) 
      break;
    p = (SYSTEM_PROCESS_INFORMATION*)((PBYTE)p + p->NextEntryOffset);
  }
  
  end_scratch(&scratch);

  return result_list;
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
  
  for (;!os_window_should_close();)
  {
    // Usage code
    Arena* arena = arena_alloc(Gigabytes(64));
    ProcessInfoList proc_list = Win32QueryProcessList(arena); // <-- USE THIS FUNCTION
    
    #define Str8Varg(S) (int)((S).count), ((S).data)     // use this for variadic functions where the format specifier is "%.*s" meaning an int value (width) is provided before the char string.


    for (auto* node = proc_list.first; node != nullptr; node = node->next)
    {
      printf("PID: %6u PPID: %6u Creation Time: %.*s\n",
            node->pid, node->ppid, Str8Varg(node->create_time));
    }

    // TODO(S): Implement proper timer
    Sleep(2000);  
  }

  return 0;
  
}


