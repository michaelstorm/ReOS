// Copyright 2007-2009 Russ Cox.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <assert.h>
#include "reos_debugger.h"
#include "reos_kernel.h"
#include "reos_stdlib.h"

static void swap_threadlists(ReOS_State *state)
{
	ReOS_ThreadList *tmp = state->current_thread_list;
	state->current_thread_list = state->next_thread_list;
	state->next_thread_list = tmp;
	state->next_thread_list->gen++;
}

ReOS_Kernel *new_reos_kernel(ReOS_Pattern *pattern, ExecuteInstFunc execute_inst,
				   int max_capturesets)
{
	ReOS_Kernel *k = calloc(1, sizeof(ReOS_Kernel));
	k->pattern = pattern;
	k->execute_inst = execute_inst;
	k->free_captureset_list = new_reos_compoundlist(32, (VoidPtrFunc)delete_reos_captureset, 0);
	k->free_thread_list = new_reos_compoundlist(32, (VoidPtrFunc)delete_reos_thread, 0);
	k->debuggers = new_reos_simplelist(0);

	k->max_capturesets = max_capturesets;
	k->matches = new_reos_simplelist((VoidPtrFunc)reos_captureset_deref);
	k->next_backref_id = 1;

	k->state.current_thread_list = new_reos_threadlist(32);
	k->state.next_thread_list = new_reos_threadlist(32);
	return k;
}

void free_reos_kernel(ReOS_Kernel *k)
{
	if (k) {
		free_reos_tokenbuffer(k->token_buf);
		free_reos_threadlist(k->state.current_thread_list);
		free_reos_threadlist(k->state.next_thread_list);
		free_reos_simplelist(k->matches);
		free_reos_simplelist(k->debuggers);
		free_reos_compoundlist(k->free_captureset_list);
		free_reos_compoundlist(k->free_thread_list);
		free(k);
	}
}

void reos_kernel_free_memory_pools(ReOS_Kernel *k)
{
	assert(free_reos_compoundlist(k->free_captureset_list) == 0);
	assert(free_reos_compoundlist(k->free_thread_list) == 0);
	k->free_captureset_list = new_reos_compoundlist(32, (VoidPtrFunc)delete_reos_captureset, 0);
	k->free_thread_list = new_reos_compoundlist(32, (VoidPtrFunc)delete_reos_thread, 0);
}

int reos_kernel_save_captureset(ReOS_Kernel *k, ReOS_CaptureSet *capture_set)
{
	if (k->max_capturesets == -1 || k->num_capturesets < k->max_capturesets) {
		k->num_capturesets++;

		reos_captureset_ref(capture_set);
		capture_set = reos_captureset_detach(capture_set);

		reos_simplelist_push_tail(k->matches, capture_set);
	}

	return 0;
}

static void push_threadlist(ReOS_Kernel *k, ReOS_ThreadList *threadlist,
								   ReOS_Thread *thread, int backtrack)
{
	if (k->pattern->get_inst(k->pattern, thread->pc))
		reos_threadlist_push_tail(threadlist, thread, backtrack);
	else
		free_reos_thread(thread);
}

void reos_kernel_push_current_threadlist(ReOS_Kernel *k, ReOS_Thread *thread, int backtrack)
{
	push_threadlist(k, k->state.current_thread_list, thread, backtrack);
}

void reos_kernel_push_next_threadlist(ReOS_Kernel *k, ReOS_Thread *thread, int backtrack)
{
	push_threadlist(k, k->state.next_thread_list, thread, backtrack);
}

int reos_kernel_step_instruction(ReOS_Kernel *k, ReOS_Thread *thread, ReOS_Inst *inst, int ops)
{
	int inst_ret = k->execute_inst(k, thread, inst, ops);

	if (inst_ret & ReOS_InstRetMatch)
		inst_ret |= reos_kernel_save_captureset(k, thread->capture_set);
	else if ((inst_ret & ReOS_InstRetDrop) && (ops & REOS_PARTIAL)
			 && (k->current_token == 0)) {
		inst_ret |= ReOS_InstRetMatch;
		inst_ret |= reos_kernel_save_captureset(k, thread->capture_set);
	}

	if (inst_ret & ReOS_InstRetConsume) {
		thread->pc++;
		reos_kernel_push_next_threadlist(k, thread, (inst_ret & ReOS_InstRetBacktrack) ? 1 : 0);
	}

	if (inst_ret & ReOS_InstRetStep) {
		thread->pc++;
		reos_kernel_push_current_threadlist(k, thread, (inst_ret & ReOS_InstRetBacktrack) ? 1 : 0);
	}

	if (inst_ret & ReOS_InstRetDrop)
		free_reos_thread(thread);
	return inst_ret;
}

int reos_kernel_step_token(ReOS_Kernel *k, int ops)
{
	swap_threadlists(&k->state);

	foreach_simple(ReOS_Debugger, bt_debugger, k->debuggers) {
		if (bt_debugger->before_token)
			bt_debugger->before_token(bt_debugger, k);
	}

	int inst_ret = ReOS_InstRetDrop;
	k->current_token = reos_tokenbuffer_read(k->token_buf);

	while (reos_compoundlist_has_next(k->state.current_thread_list->list)) {
		foreach_simple(ReOS_Debugger, bi_debugger, k->debuggers) {
			if (bi_debugger->before_inst)
				bi_debugger->before_inst(bi_debugger, k);
		}

		ReOS_Thread *thread = reos_threadlist_pop_head(k->state.current_thread_list);
		if (!thread)
			return 0;

		ReOS_Inst *inst = k->pattern->get_inst(k->pattern, thread->pc);
		inst_ret = reos_kernel_step_instruction(k, thread, inst, ops);

		foreach_simple(ReOS_Debugger, ai_debugger, k->debuggers) {
			if (ai_debugger->after_inst)
				ai_debugger->after_inst(ai_debugger, k);
		}

		if (inst_ret & ReOS_InstRetHalt)
			break;
	}

	foreach_simple(ReOS_Debugger, at_debugger, k->debuggers) {
		if (at_debugger->after_token)
			at_debugger->after_token(at_debugger, k);
	}

	k->sp++;
	return inst_ret;
}

int reos_kernel_execute(ReOS_Kernel *k, ReOS_Input *input, int start_offset, int ops)
{
	free_reos_tokenbuffer(k->token_buf);
	k->token_buf = new_reos_tokenbuffer(input);

	if (ops & REOS_BACKTRACK_MATCHING) {
		k->state.current_thread_list->backtrack_captures = 1;
		k->state.next_thread_list->backtrack_captures = 1;
	}

	k->sp = start_offset;
	reos_tokenbuffer_fastforward(k->token_buf, start_offset);
	reos_kernel_bootstrap(k, 0);

	foreach_simple(ReOS_Debugger, start_debugger, k->debuggers) {
		if (start_debugger->start)
			start_debugger->start(start_debugger, k);
	}

	int inst_ret;
	while (reos_compoundlist_has_next(k->state.next_thread_list->list)) {
		inst_ret = reos_kernel_step_token(k, ops);
		if (inst_ret & ReOS_InstRetHalt)
			break;
		
		if (k->current_token && !(ops & REOS_ANCHORED))
			reos_kernel_bootstrap(k, 0);
	}

	foreach_simple(ReOS_Debugger, end_debugger, k->debuggers) {
		if (end_debugger->end)
			end_debugger->end(end_debugger, k);
	}

	return k->num_capturesets;
}

void reos_kernel_bootstrap(ReOS_Kernel *k, int pc)
{
	ReOS_Thread *start_thread = new_reos_thread(k->free_thread_list, pc);
	start_thread->capture_set = new_reos_captureset(k->free_captureset_list);
	reos_threadlist_push_tail(k->state.next_thread_list, start_thread, 0);
}
