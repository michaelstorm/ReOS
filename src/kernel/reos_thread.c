#include <assert.h>
#include <Judy.h>
#include <string.h>
#include "reos_capture.h"
#include "reos_list.h"
#include "reos_stdlib.h"
#include "reos_thread.h"

typedef struct ThreadEntry ThreadEntry;

struct ThreadEntry
{
	long gen;
	long capture_set_version;
	ReOS_CaptureSet *capture_set;
	ReOS_CompoundList *deps;
};

static int thread_alive(ReOS_Thread *, long gen);
static void thread_ref_branches(ReOS_Thread *);
static void thread_deref_branches(ReOS_Thread *);

static void free_threadentry(ThreadEntry *entry)
{
	if (entry) {
		free_reos_compoundlist(entry->deps);
		free(entry);
	}
}

ReOS_Thread *new_reos_thread(ReOS_CompoundList *free_thread_list, int pc)
{
	ReOS_Thread *t;
	if (reos_compoundlist_has_next(free_thread_list))
		t = reos_compoundlist_pop_head(free_thread_list);
	else {
		t = malloc(sizeof(ReOS_Thread));
		t->free_thread_list = free_thread_list;

		t->ref = 0;
		t->deps = 0;
	}

	t->pc = pc;
	t->backref_buffer = 0;
	return t;
}

void free_reos_thread(ReOS_Thread *t)
{
	if (t) {
		if (t->capture_set)
			reos_captureset_deref(t->capture_set);

		free_reos_backrefbuffer(t->backref_buffer);

		if (t->deps) {
			free_reos_compoundlist(t->deps);
			t->deps = 0;
		}
		if (t->ref) {
			reos_branch_strong_deref(t->ref);
			t->ref = 0;
		}

		reos_compoundlist_push_tail(t->free_thread_list, t);
	}
}

void delete_reos_thread(ReOS_Thread *t)
{
	free(t);
}

ReOS_Thread *reos_thread_clone(ReOS_Thread *t)
{
	ReOS_Thread *clone = new_reos_thread(t->free_thread_list, t->pc);
	clone->capture_set = t->capture_set;
	clone->backref_buffer = t->backref_buffer;
	reos_captureset_ref(clone->capture_set);

	if (t->deps)
		clone->deps = reos_compoundlist_clone(t->deps);
	if (t->ref) {
		reos_branch_strong_ref(t->ref);
		clone->ref = t->ref;
	}
	return clone;
}

ReOS_ThreadList *new_reos_threadlist(int len)
{
	ReOS_ThreadList *l = malloc(sizeof(ReOS_ThreadList));
	l->list = new_reos_compoundlist(len, (VoidPtrFunc)free_reos_thread, 0);
	l->pc_table = 0;
	l->backtrack_captures = 0;
	l->gen = 1;
	return l;
}

void free_reos_threadlist(ReOS_ThreadList *l)
{
	if (l) {
		free_reos_compoundlist(l->list);
		free_judy(l->pc_table, (VoidPtrFunc)free_threadentry);
		free(l);
	}
}

static int can_insert_thread(ReOS_ThreadList *l, ReOS_Thread *t)
{
	judy_fail_get(l->pc_table, t->pc, ThreadEntry, entry) {
		entry = calloc(1, sizeof(ThreadEntry));
		judy_insert(l->pc_table, t->pc, entry);
	}

	int insert = 0;
	if (entry->gen < l->gen)
		insert = 1;
	else if (l->backtrack_captures) {
		if (entry->capture_set == t->capture_set)
			insert = (t->capture_set->version != entry->capture_set_version);
		else
			insert = 1;

		if (!insert) {
			if (t->deps && entry->deps && entry->deps->impl != t->deps->impl)
				insert = 1;
			else if (t->deps && !entry->deps)
				insert = 1;
			else if (entry->deps && !t->deps)
				insert = 1;
		}
	}

	if (insert) {
		entry->gen = l->gen;
		if (l->backtrack_captures) {
			entry->capture_set = t->capture_set;
			entry->capture_set_version = t->capture_set->version;

			// prevent t->deps from being freed and re-allocated later, so the
			// pointer value will erroneously match
			if (entry->deps)
				free_reos_compoundlist(entry->deps);

			if (t->deps)
				entry->deps = reos_compoundlist_clone(t->deps);
			else
				entry->deps = 0;
		}
	}

	return insert;
}

void reos_threadlist_push_head(ReOS_ThreadList *l, ReOS_Thread *t, int backtrack)
{
	if (backtrack || can_insert_thread(l, t)) {
		reos_compoundlist_push_head(l->list, t);
		thread_ref_branches(t);
	}
	else
		free_reos_thread(t);
}

void reos_threadlist_push_tail(ReOS_ThreadList *l, ReOS_Thread *t, int backtrack)
{
	if (backtrack || can_insert_thread(l, t)) {
		reos_compoundlist_push_tail(l->list, t);
		thread_ref_branches(t);
	}
	else
		free_reos_thread(t);
}

ReOS_Thread *reos_threadlist_pop_head(ReOS_ThreadList *l)
{
	ReOS_Thread *next = reos_compoundlist_peek_head(l->list);
	while (!thread_alive(next, l->gen)) {
		thread_deref_branches(next);
		free_reos_thread(next);

		reos_compoundlist_pop_head(l->list);
		if (!reos_compoundlist_has_next(l->list))
			return 0;
		else
			next = reos_compoundlist_peek_head(l->list);
	}

	ReOS_Thread *t = reos_compoundlist_pop_head(l->list);
	thread_deref_branches(t);
	return t;
}

static int thread_alive(ReOS_Thread *t, long gen)
{
	if (!t->deps)
		return 1;

	foreach_compound(ReOS_Branch, branch, t->deps) {
		if (!reos_branch_alive(branch))
			return 0;
	}

	return 1;
}

static void thread_ref_branches(ReOS_Thread *t)
{
	if (t->ref)
		t->ref->num_threads++;
}

static void thread_deref_branches(ReOS_Thread *t)
{
	if (t->ref) {
		if (t->ref->num_threads > 0)
			t->ref->num_threads--;
	}
}

ReOS_Branch *new_reos_branch(int neg)
{
	ReOS_Branch *b = malloc(sizeof(ReOS_Branch));
	b->strong_refs = 0;
	b->weak_refs = 0;
	b->num_threads = 0;
	b->matched = 0;
	b->negated = neg;
	b->marked = 0;
	b->matches = new_reos_compoundlist(4, (VoidPtrFunc)free_reos_compoundlist, 0);

	return b;
}

static void free_reos_branch(ReOS_Branch *branch)
{
	free_reos_compoundlist(branch->matches);
	free(branch);
}

/* We can't use simple reference counting with branches, since branches can refer
   to themselves multiple times in branch->matches. So we use strong and weak
   references, borrowing terms from the literature and using different definitions,
   in hope of maximal confusion. Threads have strong references to their ref branch
   and deps branch list. Branches have weak references to their matches branch
   list. When a branch's strong reference count reaches zero, it deletes a weak
   reference to every branch in its matches. This is key to eliminating circular
   references. When a branch's strong and weak reference counts reach zero, the
   branch is freed. It is important that free_reos_branch() does not decrement any
   reference counts, to ensure that we do not create a circle of free() calls.
   Note that unlike in the literature, both strong and weak references prevent a
   branch from being freed. The idea is to create a tree in which threads can refer
   to branches, but branches can only refer to themselves. So the strong reference
   count is how many threads are directly watching you, while the weak reference
   count is how many threads are indirectly watching you via other branches. In
   this way, branch references are not self-sustaining, and killing off threads
   always results in their dependent branches eventually dying. This tree metaphor
   is getting a little strong.
 */
ReOS_Branch *reos_branch_strong_ref(ReOS_Branch *branch)
{
	branch->strong_refs++;
	return branch;
}

ReOS_Branch *reos_branch_weak_ref(ReOS_Branch *branch)
{
	branch->weak_refs++;
	return branch;
}

void reos_branch_strong_deref(ReOS_Branch *branch)
{
	branch->strong_refs--;
	
	if (branch->strong_refs == 0) {
		// only free in this function if weak_refs is already zero
		int del = 0;
		if (branch->weak_refs == 0)
			del = 1;
		
		// otherwise, it will get freed if it becomes zero through a
		// circular reference in weak_deref()
		foreach_compound(ReOS_CompoundList, match_list, branch->matches) {
			foreach_compound(ReOS_Branch, b, match_list)
				reos_branch_weak_deref(b);
		}
		
		if (del)
			free_reos_branch(branch);
	}
}

void reos_branch_weak_deref(ReOS_Branch *branch)
{
	branch->weak_refs--;
	if (branch->weak_refs == 0 && branch->strong_refs == 0)
		free_reos_branch(branch);
}

void reos_branch_print(ReOS_Branch *b)
{
	if (b->negated)
		printf("(!)");
	if (b->marked)
		printf("(*)");
	
	if (b->matched)
		printf("%p:m[%d,%d]", b, b->strong_refs, b->weak_refs);
	else
		printf("%p:%d[%d,%d]", b, b->num_threads, b->strong_refs, b->weak_refs);
}

int reos_branch_alive(ReOS_Branch *branch)
{
	if (branch->negated) {
		if (branch->matched)
			return 0;
	}
	else if (!branch->num_threads && !branch->matched)
		return 0;

	return 1;
}

int reos_branch_succeeded(ReOS_Branch *branch)
{
	if (branch->negated) {
		if (branch->num_threads > 0 || branch->matched)
			return 0;
	}
	else if (!branch->matched)
		return 0;

	return 1;
}
