/*  date = July 09th 2026  */

/*
NOTE(S): Not using any of your abstractions yet, going super simple only using the base layer. When I have something I'm happy with, I'll integrate with your os layer.
TODO(S): Need to use same tab size, also I think my editor doesn't sub out spaces for tabs.
*/

void OutputDebugStringF(const char* fmt, ...);

#include "core/core_include.h"
#include "core/core_include.cpp"


/*
Brain dump.

---Session Semantics----
Session: 
- continuous period where an application's process exists
Session starts when: 
- a process first appears.
Sessions ends when:
- process exits
- pc shuts down/crashes (recovered on next launch via temp session at hte last heartbeat)
- (will htink about later) app has been inactive for a long period to support suspension

--- Store per Session ----
- process name
- exe path/name
- icon
- pid
- ppid
- start timestamp
- end timestamp
- duration
- peak cpu usage		( we can think later how to do the graph )
- peak gpu usage
- peak memory usage
- focused duration (optional)
- background duration (optional)
- recovered from crash flag

---- Persistence ----
When a session starts:
- create a temp session file
Every 5-10 seconds (can expose this to user)
- update elapsed duration
- update peak CPU/GPU/MEM
- flush to disk
When session ends:
- finalise session and make it permament
- delete temp file
On application startup:
- look for unfinished temp sessions
- recover them
- end them at the last heartbeat timestamp
- mark as recovered_from_crash

---- UI ----
Default view should be daily totals.
e.g.
16 Apr 2026
Fortnite 4h 12min
VS Code  2h 18min
Spotify  1h 05min

These can be expanded if needed to show indivual sessions in the day
e.g.
Fortnite
18:02 → 19:10   1h 08m
19:22 → 21:05   1h 43m
21:20 → 22:14   54m

The user should be able to aggregrate based on daily/weekly/monthly.

---- Short Sessions ----
Should track every session internally and only display sessions if their duration >= 30 seconds.
However, very short launches can be either hidden or appear as "N quick launches"

---- Session merging rules ----
Merge consecutive sessions when:
- same exe path AND
- new process starts withing 10-30 seconds AND
- previous process wasn't intentionally closed (updates might change exe path, so treat as new app?)
Why?
- to avoid launcher/game restarts becoming multiple sessions
- to avoid update or crashes splitting one play session

Merge simulatenous sessions when:
- treat as one application session
- track all pids of the same exe
- end session when last pid exits

---- Edge Cases ----
Crash or power loss
- recover unifinished session from temp file
- end session at last saved heartbeat
App restarts itself
- merge into previous session if merge rules match
e.g.
Game.exe
|
V
Game exits
|	(4 seconds)
V
Game.exe starts again

= one session

Launcher starts game. 
- E.g. steam starts game.exe. Need to track seperately. Might be a non-issue and should just work.

Midnight
- don't split sessions
- store 23:30 -> 01:15
- when displaying daily sesisons, do the calculation yourself
e.g. 
15 Apr = 30 min
16 Apr = 1h 15min

System Sleep
- record sleep start
- record wake time
Either
- exclude sleep from duration ( simple, i like this)
- make it user configurable (can think about later)

Clock changes
- store timestamps in UTC
- convert to local time only when displaying

Executable moved
- identify apps by exe path initially
- later I can introduce a stable app id so moving/installing doesnt create a "new" app (whenever an app is opened for the first time i get its name and give it an id? will think about this later)

Renamed exe
- if exe name/path changed after an update
-- consider as new app
-- look into "smarter" matching later


*/

#define MAX_PATH_SIZE 32767

int main()
{
  // TODO(S): Support Unicode!

  allocate_thread_context();
  B32 os_init_succ = os_init();
  if (!os_init_succ) { return -1; }
  OS_State* win32_state = os_get_state();
  
  // NOTE(S): 
  char exe_path[MAX_PATH_SIZE];
  DWORD exe_path_size{MAX_PATH_SIZE};

  for (;!os_window_should_close();)
  {
    exe_path_size = 0;
    
    // get window handle of window user is interacting with (you change foreground window with Window+Alt+Tab or just clicking on window)
    HWND foreground_hwnd = GetForegroundWindow();
    if (!foreground_hwnd)
    {
      printf("GetForegroundWindow() err");
      return 0;
    }

    // get pid
    DWORD pid{};
    GetWindowThreadProcessId(foreground_hwnd, &pid);  // Note(S): returns thread id

    HANDLE p_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!p_handle)
    {
      printf("OpenProcess err");  // Note(S): May crash because process is more elevated than ours, may crash for other reasons. All that can just be ignored. We don't care about processes that are elevated. If the user runs this app as admin, then we will capture them, simple. Or we can ignore by checking their access token, and if its admin we ignore again.
    }
    
    if (!QueryFullProcessImageNameA(p_handle, 0, exe_path, &exe_path_size))
    {
      printf("QueryFullProcessImageNameA err");
    }

    // TODO(S): Get display name. 
    // This is a little more involved, and will require using the native api.
    // For the app/display name, I need to get full exe path, and read its FileDescription (from the PE i think)
    // if its a packaged app, read its manifest display name.

    // TODO(S): Get memory/cpu/gpu usage. 
    // All of these are not directly queryable. I have to calculate this. 
    // memory and cpu seem doable, but I am not sure about gpu, I think it will require using directx.
    
    // print
    printf("exe path: %s\n", exe_path);

    // TODO(S): Implement proper timer
    Sleep(2000);  
  }



return 0;
}

///////////////////////////////////////////////////////////
// - Main helpers
//
// todo: Code for this is bad
void OutputDebugStringF(const char* fmt, ...)
{
  #if DEBUG_MODE
  va_list argptr;
  va_start(argptr, fmt);
  Scratch scratch = get_scratch(0, 0);
  Data_buffer buffer = data_buffer_make(scratch.arena, 128);
  int ret = vsprintf_s((char*)buffer.data, buffer.count, fmt, argptr);
  if (ret >= 0 && ret < buffer.count)
  {
    OutputDebugStringA((char*)buffer.data);
  } else { InvalidCodePath(); }
  end_scratch(&scratch);
  va_end(argptr);
  #endif
}
