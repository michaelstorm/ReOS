#ifndef PROFILE_DEBUGGER_H
#define PROFILE_DEBUGGER_H

#include "reos_debugger.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ProfileDebuggerData ProfileDebuggerData;

struct ProfileDebuggerData
{
	clock_t start_time;
	clock_t end_time;
	int instrs_executed;
	int tokens_read;
	int max_threads;
};

ReOS_Debugger *new_profile_debugger();
void free_profile_debugger(ReOS_Debugger *);
void profile_debugger_start_timer(ReOS_Debugger *, ReOS_Kernel *);
void profile_debugger_stop_timer(ReOS_Debugger *, ReOS_Kernel *);
void profile_debugger_count_inst(ReOS_Debugger *, ReOS_Kernel *);
void profile_debugger_count_token(ReOS_Debugger *, ReOS_Kernel *);
void profile_debugger_print_results(ReOS_Debugger *);

#ifdef __cplusplus
}
#endif

#endif
