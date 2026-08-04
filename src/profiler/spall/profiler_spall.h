#ifndef PROFILER_SPALL_H
#define PROFILER_SPALL_H

// DD: This this the cpp+h file for spall, its all in 1 file
#pragma warning(disable: 4267)
#pragma warning(disable: 4244)
#pragma warning(disable: 4996)
#include "__third_party/spall/spall.h"
#pragma warning(default: 4267)
#pragma warning(default: 4244)
#pragma warning(default: 4996)

struct __Prof_Spall_State {
  SpallProfile spall_ctx;
  SpallBuffer  spall_buffer;
};

__Prof_Spall_State* __prof_spall_get_state()
{
  Prof_State* prof_state = prof_get_state();
  U64 addr_for_spall = prof_state->addr_to_specific_backend_struct;
  __Prof_Spall_State* spall_backend_state = (__Prof_Spall_State*)addr_for_spall;
  return spall_backend_state;
}

U64 __prof_spall_get_time_in_ns();

#endif