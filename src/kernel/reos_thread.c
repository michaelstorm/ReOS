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

		t->refs = 0;
		t->deps = 0;
	}

	t->pc = pc;
	t->backref_buffer = 0;
	t->join_root = 0;
	return t;
}

void free_reos_thread(ReOS_Thread *t)
{
	if (t) {
		if (t->capture_set)
			reos_captureset_deref(t->capture_set);

		free_reos_backrefbuffer(t->backref_buffer);
		free_reos_joinroot(t->join_root);

		if (t->deps) {
			free_reos_compoundlist(t->deps);
			t->deps = 0;
		}
		if (t->refs) {
			free_reos_compoundlist(t->refs);
			t->refs = 0;
		}

		reos_compoundlist_push_tail(t->free_thread_list, t);
	}
}

void delete_reos_thread(ReOS_Thread *t)
{
	if (t)
		free(t);
}

ReOS_Thread *reos_thread_clone(ReOS_Thread *t)
{
	ReOS_Thread *clone = new_reos_thread(t->free_thread_list, t->pc);
	clone->capture_set = t->capture_set;
	clone->join_root = t->join_root;
	clone->backref_buffer = t->backref_buffer;
	reos_captureset_ref(clone->capture_set);

	if (t->join_root)
		clone->join_root = joinroot_clone(t->join_root);

	if (t->deps)
		clone->deps = reos_compoundlist_clone(t->deps);
	if (t->refs)
		clone->refs = reos_compoundlist_clone(t->refs);
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
	if (t->refs) {
		foreach_compound(ReOS_Branch, branch, t->refs)
			branch->num_threads++;
	}
}

static void thread_deref_branches(ReOS_Thread *t)
{
	if (t->refs) {
		foreach_compound(ReOS_Branch, branch, t->refs) {
			if (branch->num_threads > 0)
				branch->num_threads--;
		}
	}
}

static void free_reos_branch_tree(ReOS_Branch *branch)
{
	if (branch) {
		free_reos_simplelist(branch->children);
		free(branch);
	}
}

ReOS_Branch *new_reos_branch(int neg)
{
	ReOS_Branch *t = malloc(sizeof(ReOS_Branch));
	t->refs = 0;
	t->num_threads = 0;
	t->matched = 0;
	t->negated = neg;
	t->marked = 0;
	t->parent = 0;
	t->children = new_reos_simplelist((VoidPtrFunc)free_reos_branch_tree);
	t->matches = 0;

	return t;
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

static ReOS_JoinRootImpl *new_reos_joinrootimpl()
{
	ReOS_JoinRootImpl *impl = malloc(sizeof(ReOS_JoinRootImpl));
	impl->refs = 1;
	impl->root_branch = 0;

	return impl;
}

ReOS_JoinRoot *new_reos_joinroot(ReOS_Branch *root_branch)
{
	ReOS_JoinRoot *root = malloc(sizeof(ReOS_JoinRoot));
	root->impl = new_reos_joinrootimpl();
	root->impl->root_branch = root_branch;

	return root;
}

void free_reos_joinroot(ReOS_JoinRoot *root)
{
	if (root) {
		if (--root->impl->refs == 0) {
			//free_reos_branch_tree(root->impl->root_branch);
			free(root->impl);
		}
		free(root);
	}
}

ReOS_JoinRoot *joinroot_clone(ReOS_JoinRoot *root)
{
	ReOS_JoinRoot *clone = malloc(sizeof(ReOS_JoinRoot));
	clone->impl = root->impl;
	clone->impl->refs++;

	return clone;
}

ReOS_Branch *joinroot_get_root_branch(ReOS_JoinRoot *root)
{
	return root->impl->root_branch;
}
