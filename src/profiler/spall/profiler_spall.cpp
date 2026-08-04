#ifndef PROFILER_SPALL_CPP
#define PROFILER_SPALL_CPP

// DD: This this the cpp+h file for spall, its all in 1 file
#pragma warning(disable: 4267)
#pragma warning(disable: 4244)
#pragma warning(disable: 4996)
#include "__third_party/spall/spall.h"
#pragma warning(default: 4267)
#pragma warning(default: 4244)
#pragma warning(default: 4996)

// TODO: Deal with include here, i dont like how you include all the random shit right now

#include "os/win32.cpp" // DD: This is for time
#include "profiler/spall/profiler_spall.h"


///////////////////////////////////////////////////////////
// - State #[Per backend]
//
void prof_init()
{
  // DD: Filling up generic state
  Arena* arena = arena_alloc(Megabytes(4));
  __prof_g_state = ArenaPush(arena, Prof_State);
  __prof_g_state->state_arena = arena;
  __Prof_Spall_State* spall_backend_memory = ArenaPush(arena, __Prof_Spall_State);
  __prof_g_state->addr_to_specific_backend_struct = (U64)spall_backend_memory;

  // DD: From here we start setting up spall backend 

  if (!spall_init_file("SPAL_PROFILING_DATA.spall", 1, &spall_backend_memory->spall_ctx)) {
		Handle(1);
    printf("Failed to setup spall?\n");
		exit(1); // Todo: Remove this 
	}

	Data_buffer buffer = data_buffer_make(__prof_g_state->state_arena, Megabytes(3));
  spall_backend_memory->spall_buffer.data   = buffer.data;
  spall_backend_memory->spall_buffer.length = buffer.count;
	if (!spall_buffer_init(&spall_backend_memory->spall_ctx, &spall_backend_memory->spall_buffer)) {
		printf("Failed to init spall buffer?\n");
		exit(1); // Todo: Remove this
	}
}

void prof_release()
{
  __Prof_Spall_State* spall_state = (__Prof_Spall_State*)__prof_g_state->addr_to_specific_backend_struct;
  spall_buffer_quit(&spall_state->spall_ctx, &spall_state->spall_buffer);
	spall_quit(&spall_state->spall_ctx);

  spall_state = 0;
  arena_release(&__prof_g_state->state_arena);
  __prof_g_state = 0;
}

///////////////////////////////////////////////////////////
// - Profiling #[Per backend] 
//
void prof_begin_prof_scope_fmt(const char* fmt, ...)
{
  __Prof_Spall_State* prof_spall_state = __prof_spall_get_state();

  Scratch scratch = get_scratch(0, 0);
	va_list argptr;
	va_start(argptr, fmt);

	Str8 str = str8_valist(scratch.arena, fmt, argptr); Assert(str.count <= s32_max);
	spall_buffer_begin(&prof_spall_state->spall_ctx, &prof_spall_state->spall_buffer, (char*)str.data, (S32)str.count, __prof_spall_get_time_in_ns());

	va_end(argptr);
	end_scratch(&scratch);
}

void prof_end_prof_scope()
{
  __Prof_Spall_State* prof_spall_state = __prof_spall_get_state();
  spall_buffer_end(&prof_spall_state->spall_ctx, &prof_spall_state->spall_buffer, __prof_spall_get_time_in_ns());
}

///////////////////////////////////////////////////////////
// - Spall back end specific helpers
///////////////////////////////////////////////////////////

U64 __prof_spall_get_time_in_ns()
{
  return (U64)os_get_time_for_timing_in_ns();
}

#endif