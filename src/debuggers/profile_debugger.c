#include <stdio.h>
#include <stdlib.h>
#include "profile_debugger.h"
#include "reos_list.h"

ReOS_Debugger *new_profile_debugger()
{
	ReOS_Debugger *d = malloc(sizeof(ReOS_Debugger));
	debugger_reset(d);

	d->start = profile_debugger_start_timer;
	d->end = profile_debugger_stop_timer;
	d->before_inst = profile_debugger_count_inst;
	d->before_token = profile_debugger_count_token;

	d->data = malloc(sizeof(ProfileDebuggerData));
	ProfileDebuggerData *data = d->data;
	data->instrs_executed = 0;
	data->tokens_read = 0;
	data->max_threads = 0;

	return d;
}

void free_profile_debugger(ReOS_Debugger *d)
{
	free(d->data);
	free(d);
}

void profile_debugger_start_timer(ReOS_Debugger *debugger, ReOS_Kernel *k)
{
	ProfileDebuggerData *data = debugger->data;
	data->start_time = clock();
}

void profile_debugger_stop_timer(ReOS_Debugger *debugger, ReOS_Kernel *k)
{
	ProfileDebuggerData *data = debugger->data;
	data->end_time = clock();
}

void profile_debugger_count_inst(ReOS_Debugger *debugger, ReOS_Kernel *k)
{
	ProfileDebuggerData *data = debugger->data;
	data->instrs_executed++;

	int threads = 0;
	foreach_compound(ReOS_Thread, t, k->state.current_thread_list->list)
		threads++;

	if (threads > data->max_threads)
		data->max_threads = threads;
}

void profile_debugger_count_token(ReOS_Debugger *debugger, ReOS_Kernel *k)
{
	ProfileDebuggerData *data = debugger->data;
	data->tokens_read++;
}

void profile_debugger_print_results(ReOS_Debugger *debugger)
{
	ProfileDebuggerData *data = debugger->data;
	printf("\nTime elapsed: %.3lf\n", ((double)(data->end_time - data->start_time)) / CLOCKS_PER_SEC);
	printf("Instructions executed: %d\n", data->instrs_executed);
	printf("Tokens read: %d\n", data->tokens_read);
	printf("Max threads: %d\n", data->max_threads);
}
