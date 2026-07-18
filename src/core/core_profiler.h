#ifndef CORE_PROFILER_H
#define CORE_PROFILER_H

#include "core/core_base.h"
#include "core/core_base.cpp" // Todo :REmove this from here

#include "core/core_arena.h"
#include "core/core_arena.cpp" // Todo: Remove this

#pragma warning(disable: 4267)
#pragma warning(disable: 4244)
#pragma warning(disable: 4996)
#include "__third_party/spall/spall.h"
#pragma warning(default: 4267)
#pragma warning(default: 4244)
#pragma warning(default: 4996)

// TODO: Need a better way to have this whole thing inited here
// TODO: Might be nice to just have an overall init for the things that we from base, like core_init
//       and there we init all the stuff that we need from all the little parts of core
global Arena* spall_arena        = {};
global SpallProfile spall_ctx    = {};
global SpallBuffer  spall_buffer = {};

void profiler_init()
{
  if (!spall_init_file("hello_world.spall", 1, &spall_ctx)) {
		printf("Failed to setup spall?\n");
		exit(1); // Todo: Remove this 
	}

	spall_arena = arena_alloc(Megabytes(4));
	Data_buffer buffer = data_buffer_make(spall_arena, Megabytes(3));
	spall_buffer = {};
	spall_buffer.data   = buffer.data;
	spall_buffer.length = buffer.count;

	if (!spall_buffer_init(&spall_ctx, &spall_buffer)) {
		printf("Failed to init spall buffer?\n");
		exit(1); // Todo: Remove this
	}
}

void profiler_release()
{
	spall_buffer_quit(&spall_ctx, &spall_buffer);
	arena_release(&spall_arena);
	spall_quit(&spall_ctx);
}

U64 profiler_time_in_ns()
{
	U64 ns = (U64)(((F64)os_get_perf_counter()) * ((F64)1000000000.0 / (F64)os_get_perf_freq_per_sec()));
	return ns;
}

// TODO: This needs a time func with nanosectods
#define BeginProfile(name_cstr) spall_buffer_begin(&spall_ctx, &spall_buffer, name_cstr, sizeof(name_cstr), profiler_time_in_ns());
#define EndProfile()   spall_buffer_end(&spall_ctx, &spall_buffer, profiler_time_in_ns());

#endif