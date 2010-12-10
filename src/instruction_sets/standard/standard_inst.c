#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "reos_kernel.h"
#include "standard_inst.h"

/**
 * \file
 * Provides common instructions that most patterns should be able to use, but
 * are not required to.
 */

/**
 * \todo Use the ReOS_CaptureSet pointer in ReOS_ThreadEntry to store ReOS_Captures from future
 * threads, so they don't get clobbered.
 * \todo Adapt the Knuth-Morris-Pratt algorithm to regular expressions, which
 * would allow us to keep from bootstrapping the pattern from the beginning on
 * every index. The tradeoff is that it requires backtracking through the input,
 * but not the pattern.
 * \todo ReOS_Pattern recursion. If we use the same syntax as the original proposal,
 * the semantics of captureing could get confusing, even though it's rather
 * natural.
 */

ReOS_Inst *standard_inst_factory(int opcode)
{
	ReOS_Inst *inst = malloc(sizeof(ReOS_Inst));
	inst->opcode = opcode;

	switch (opcode) {
	case OpSaveStart:
	case OpSaveEnd:
	case OpBacktrack:
	case OpJmp:
	case OpSplit:
	case OpMatch:
	case OpBranch:
	case OpNegBranch:
	case OpRecurse:
		inst->args = malloc(sizeof(StandardInstArgs));
		break;

	case OpAny:
	case OpStart:
	case OpEnd:
		inst->args = 0;
		break;

	default:
		fprintf(stderr, "error: unrecognized standard opcode %d\n", opcode);
		exit(0);
		break;
	}

	return inst;
}

/**
 * The following is no longer correct:
 * Since more than one capture range can correspond to \a capture_num, this
 * function tries to match all of them and then decides how to proceed to the
 * next input token after all have been tried. There can be one or more of three
 * possible outcomes:
 * -# If any of the capture ranges have further tokens to match, the buffer
 * indexes are updated and a clone of \a thread is appended to \c
 * next_thread_list.
 * -# If any of the capture ranges have matched all their tokens, the buffer
 * is deleted and a clone of \a thread is appended to \c next_thread_list.
 * -# In the special case of a zero-length range, the buffer is deleted and a
 * clone of \a thread is appended to \c current_thread_list.
 * \todo Fix this documentation, since there is now only one capture range per
 * capture_num.
 */
static int execute_backtrack(ReOS_Kernel *k, ReOS_Thread *thread, int capture_num)
{
	ReOS_Capture *cap = reos_captureset_get_capture(thread->capture_set, capture_num);

	// skip over partial ranges
	if (cap->start == -1 || cap->end == -1)
		return ReOS_InstRetDrop;

	// if we've just started matching against this backreference, we have to
	// create a buffer to hold the tokens to test and state
	if (!thread->backref_buffer)
		thread->backref_buffer = new_reos_backrefbuffer(cap, k->token_buf->input);

	// zero length ranges automatically succeed
	if (thread->backref_buffer->length == 0) {
		free_reos_backrefbuffer(thread->backref_buffer);
		thread->backref_buffer = 0;
		return ReOS_InstRetStep;
	}
	// otherwise test the token at the current position of this buffer against
	// the input token
	else if (k->test_backref(k, k->current_token, &((char *)thread->backref_buffer->tokens)[(thread->backref_buffer->index++) * k->token_buf->input->token_size])) {
		// there are more tokens to match
		if (thread->backref_buffer->index < thread->backref_buffer->length) {
			reos_kernel_push_next_threadlist(k, thread, 1);
			return 0;
		}
		// buffer matching has succeeded
		else {
			free_reos_backrefbuffer(thread->backref_buffer);
			thread->backref_buffer = 0;
			return ReOS_InstRetConsume;
		}
	}
	// the token failed to match
	else
		return ReOS_InstRetDrop;
}

static int execute_branch(ReOS_Kernel *k, ReOS_Thread *thread, int jmp_pc,
						int join_pc, int negated)
{
	// create deps list on demand
	if (!thread->deps) {
		thread->deps = new_reos_compoundlist(4, (VoidPtrFunc)reos_branch_strong_deref, (CloneFunc)reos_branch_strong_ref);
		reos_compoundlist_unshare(thread->deps);
	}

	ReOS_Thread *join = reos_thread_clone(thread);
	join->pc = join_pc;
	thread->pc = jmp_pc;

	if (!thread->ref) {
		thread->ref = new_reos_branch(0);
		reos_branch_strong_ref(thread->ref);
	}
	
	reos_compoundlist_push_tail(join->deps, thread->ref);
	reos_branch_strong_ref(thread->ref);
	
	if (join->ref)
		reos_branch_strong_deref(join->ref);

	join->ref = new_reos_branch(negated);
	reos_branch_strong_ref(join->ref);

	reos_compoundlist_push_tail(thread->deps, join->ref);
	reos_branch_strong_ref(join->ref);

	reos_threadlist_push_head(k->state.current_thread_list, thread, 0);
	reos_threadlist_push_head(k->state.current_thread_list, join, 0);

	return 0;
}

static int check_match_list(ReOS_CompoundList *deps, int tabs)
{
#ifdef BRANCH_DEBUG
	printf("{");
	foreach_compound(ReOS_Branch, d, deps) {
		reos_branch_print(d);
		printf(", ");
	}
	printf("}\n");
#endif
	
	int ret = -1;
	
	foreach_compound(ReOS_Branch, b, deps) {
#ifdef BRANCH_DEBUG
		int i;
		for (i = 0; i < tabs; i++)
			printf("\t");
		printf("checking ");
		
		reos_branch_print(b);
		printf(" ");
#endif
		
		if (!reos_branch_succeeded(b)) {
			ret = 0;
			BRANCH_DEBUG_DO(printf("\n"));
			break;
		}
		else if (b->marked) {
			ret = 1;
			BRANCH_DEBUG_DO(printf("\n"));
			break;
		}
		
		if (!b->negated) {
			int dep_succeeded = 0;
			b->marked = 1;
			
			assert(reos_compoundlist_length(b->matches) > 0);
			BRANCH_DEBUG_DO(printf("-> "));
			foreach_compound(ReOS_CompoundList, match_list, b->matches) {
				if (check_match_list(match_list, tabs+1)) {
					dep_succeeded = 1;
					break;
				}
			}
			
			b->marked = 0;
			if (!dep_succeeded) {
				ret = 0;
				break;
			}
		}
		else
			BRANCH_DEBUG_DO(printf("\n"));
	}

#ifdef BRANCH_DEBUG	
	int i;
	for (i = 0; i < tabs; i++)
		BRANCH_DEBUG_DO(printf("\t"));
#endif
	
	if (ret == -1 || ret == 1) {
		BRANCH_DEBUG_DO(printf("succeeded\n"));
		return 1;
	}
	else {
		BRANCH_DEBUG_DO(printf("failed\n"));
		return 0;
	}
}

int execute_standard_inst(ReOS_Kernel *k, ReOS_Thread *thread, ReOS_Inst *inst, int ops)
{
	StandardInstArgs *args = (StandardInstArgs *)inst->args;

	switch (inst->opcode) {
	case OpAny:
		if (k->current_token == 0)
			return ReOS_InstRetDrop;
		else
			return ReOS_InstRetConsume;

	case OpMatch:
		if (thread->ref) {
			thread->ref->matched = 1;
			
			ReOS_CompoundList *deps_clone = reos_compoundlist_clone_with_func(thread->deps, (CloneFunc)reos_branch_weak_ref);
			deps_clone->impl->clone_element = (CloneFunc)reos_branch_weak_ref;
			deps_clone->impl->destructor = 0;
			reos_compoundlist_push_tail(thread->ref->matches, deps_clone);
			
			foreach_compound(ReOS_CompoundList, match_list, thread->ref->matches) {
				BRANCH_DEBUG_DO(printf("matched "));
				BRANCH_DEBUG_DO(reos_branch_print(thread->ref));
				BRANCH_DEBUG_DO(printf(" -> "));
				
				if (check_match_list(match_list, 1)) {
					BRANCH_DEBUG_DO(printf("succeeded\n\n"));
					return ReOS_InstRetMatch | ReOS_InstRetDrop;
				}
			}
			
			BRANCH_DEBUG_DO(printf("failed\n\n"));
			return ReOS_InstRetDrop;
		}
		else
			return ReOS_InstRetMatch | ReOS_InstRetDrop;

	case OpJmp:
		thread->pc = args->x;
		reos_threadlist_push_head(k->state.current_thread_list, thread, 0);
		return 0;

	case OpSplit:
	{
		thread->pc = args->x;

		// clone the thread
		ReOS_Thread *split = reos_thread_clone(thread);
		split->pc = args->y;

		// reuse the original and stick both back on the current thread list
		reos_threadlist_push_head(k->state.current_thread_list, split, 0);
		reos_threadlist_push_head(k->state.current_thread_list, thread, 0);
		return 0;
	}

	case OpSaveStart:
		reos_captureset_save_start(&thread->capture_set, args->x, k->sp);
		return ReOS_InstRetStep;

	case OpSaveEnd:
		reos_captureset_save_end(&thread->capture_set, args->x, k->sp);
		return ReOS_InstRetStep;

	case OpBacktrack:
		return execute_backtrack(k, thread, args->x);

	case OpStart:
		if (k->sp == 0)
			return ReOS_InstRetStep;
		else
			return ReOS_InstRetDrop;

	case OpEnd:
		if (k->current_token == 0)
			return ReOS_InstRetStep;
		else
			return ReOS_InstRetDrop;

	case OpBranch:
		return execute_branch(k, thread, args->y, args->x, 0);

	case OpNegBranch:
		return execute_branch(k, thread, args->y, args->x, 1);

//	case OpRecurse:
	//	return ReOS_InstRet
	}

	fprintf(stderr, "error: unrecognized standard opcode %d\n", inst->opcode);
	return ReOS_InstRetHalt;
}

void print_standard_inst(ReOS_Inst *inst)
{
	StandardInstArgs *args = (StandardInstArgs *)inst->args;

	switch (inst->opcode) {
	case OpSplit:
		printf("split %d, %d", args->x, args->y);
		break;

	case OpJmp:
		printf("jmp %d", args->x);
		break;

	case OpAny:
		printf("any");
		break;

	case OpMatch:
		printf("match");
		break;

	case OpSaveStart:
		printf("save-start %d", args->x);
		break;

	case OpSaveEnd:
		printf("save-end %d", args->x);
		break;

	case OpBacktrack:
		printf("backtrack %d", args->x);
		break;

	case OpStart:
		printf("start");
		break;

	case OpEnd:
		printf("end");
		break;

	case OpBranch:
		printf("branch %d, %d", args->x, args->y);
		break;

	case OpNegBranch:
		printf("neg-branch %d, %d", args->x, args->y);
		break;

	default:
		printf("unknown-opcode %d", inst->opcode);
		break;
	}
}
