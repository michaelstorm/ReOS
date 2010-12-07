// Copyright 2007-2009 Russ Cox.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <assert.h>
#include <Judy.h>
#include <string.h>
#include "reos_buffer.h"
#include "reos_capture.h"
#include "reos_list.h"
#include "reos_stdlib.h"

#define USE_MATCH_FREE_LIST

/**
 * \file
 *
 * Contains functions that operate on ReOS_CaptureSet and ReOS_Capture objects.
 *
 * A ReOS_CaptureSet object contains an interval of indexes of the input
 * over which a pattern was matched. The interval can be open or closed. If the
 * interval is closed, the full pattern was matched. If the interval is open,
 * the pattern was partially matched. See below for an explanation of partial
 * matching. ReOS_CaptureSetes always have a starting index; it is meaningless to
 * partially match backwards. A partial match has an ending index of -1.
 *
 * A ReOS_CaptureSet object can also contain ReOS_Capture objects. They are stored and
 * retrieved by their capture number, which can be anything within [0, 2^32).
 * If the ending index is not saved before the entire ReOS_CaptureSet is, it can be
 * regarded as a partial capture. An unsaved index is represented as -1. The
 * API provides no protection, other than a few assert()'s, from saving a start
 * index higher than an end index, or from saving only an end index. It is the
 * application's responsibility to deal with any partial captures.
 *
 * ReOS_Captures can be nested within the expression arbitrarily, but since they
 * are referenced only by number, such nesting is irrelevant to the kernel. For
 * example, if we run <tt>./bin/ascii -m '(a((b)c))' abc</tt>, the tree
 * produced is:
 *
\verbatim
Paren(0, Cat(Char('a'), Paren(1, Cat(Paren(2, Char('b')), Char('c')))))
\endverbatim
 *
 * and the pattern is:
 *
\verbatim
0. save-start 0
1. char a
2. save-start 1
3. save-start 2
4. char b
5. save-end 2
6. char c
7. save-end 1
8. save-end 0
9. match 0
\endverbatim
 *
 * which outputs:
 *
\verbatim
| ReOS_CaptureSet	Sub	Num	Indexes	Reconstruction
+ -----
| 0
|	+ -----
|	| 0
|	|	+ -----
|	|	| 0	[0-3]	abc
|	|	+ -----
|	+ -----
|	| 1
|	|	+ -----
|	|	| 0	[1-3]	bc
|	|	+ -----
|	+ -----
|	| 2
|	|	+ -----
|	|	| 0	[1-2]	b
|	|	+ -----
|	+ -----
+ -----
\endverbatim
 *
 * The \c captures struct member is actually an array of arrays. Since a
 * capture can be valid more than once per match, we have to be careful about
 * our bookkeeping. For example, running <tt>./bin/ascii -m -b '(a)*' aa</tt>
 * produces:
 *
\verbatim
| ReOS_CaptureSet	Sub	Num	Indexes	Reconstruction
+ -----
| 0
+ -----
| 1			[0-1]	a
|	+ -----
|	| 0
|	|	+ -----
|	|	| 0	[0-1]	a
|	|	+ -----
|	+ -----
+ -----
| 2
+ -----
| 3
|	+ -----
|	| 0
|	|	+ -----
|	|	| 0	[0-1]	a
|	|	+ -----
|	|	| 1	[1-2]	a
|	|	+ -----
|	+ -----
+ -----
| 4
|	+ -----
|	| 0
|	|	+ -----
|	|	| 0	[1-2]	a
|	|	+ -----
|	+ -----
+ -----
| 5
+ -----
\endverbatim
 *
 * The \c captures struct member is indexed by capture number, and represents
 * the list of captures as the user of the pattern would be likely to see
 * them. Each object in this top-level list is another list of every interval
 * within the match interval for which this capture is valid. These secondary
 * lists are indexed by \c capture_id, which is arbitrary, but their order of
 * iteration is constant.
 */

/**
 * Returns an available ReOS_CaptureSet object.
 *
 * Every ReOS_CaptureSet object has a \a free_list associated with it. If \c
 * USE_MATCH_FREE_LIST is defined, when the ReOS_CaptureSet's reference count becomes zero
 * and the ReOS_CaptureSet is freed, the object is added to its \a free_list.
 * Subsequently, the next time new_reos_captureset() is called with that \a free_list, the
 * old object is returned instead of requiring a call to malloc(). If \c
 * USE_MATCH_FREE_LIST is not defined, malloc() is called every time.
 *
 * \param start The input index at which this match begins
 * \param free_list The \a free_list from which to retrieve used ReOS_CaptureSet
 * objects
 *
 * \todo Make the freelist funtionality more advanced; a lot more time could be
 * saved by also keeping ReOS_Capture freelists.
 */
ReOS_CaptureSet *new_reos_captureset(ReOS_CompoundList *free_list)
{
	ReOS_CaptureSet *set;
#ifdef USE_MATCH_FREE_LIST
	if (reos_compoundlist_has_next(free_list)) {
		set = reos_compoundlist_pop_tail(free_list);
		set->version++; // don't confuse its identity with its old value
	}
	else
#endif
	{
		set = malloc(sizeof(ReOS_CaptureSet));
		set->captures = 0;
		set->version = 0;
		set->free_list = free_list;
	}

	set->refs = 1;
	return set;
}

/**
 * Deletes or appends a ReOS_CaptureSet object to its \c free_list.
 *
 * If \c USE_MATCH_FREE_LIST is defined, the ReOS_CaptureSet object's captures are freed
 * and the object itself is appended to its \c free_list. Otherwise, this
 * function simply calls delete_reos_captureset().
 *
 * \param m The object to free
 */
void free_reos_captureset(ReOS_CaptureSet *set)
{
#ifdef USE_MATCH_FREE_LIST
	if (set) {
		free_reos_judylist(set->captures);
		set->captures = 0; // if m is deleted later, don't re-free captures

		reos_compoundlist_push_tail(set->free_list, set);
	}
#else
	delete_reos_captureset(set);
#endif
}

/**
 * Permanently deletes a ReOS_CaptureSet object.
 *
 * This function first frees all of its ReOS_Capture objects, then deletes the ReOS_CaptureSet
 * object itself.
 *
 * \param m The object to delete
 */
void delete_reos_captureset(ReOS_CaptureSet *set)
{
	if (set) {
		free_reos_judylist(set->captures);
		free(set);
	}
}

/**
 * Creates a new ReOS_Capture object.
 *
 * \return A new ReOS_Capture object with its \c start and \c end members
 * initialized to -1.
 */
ReOS_Capture *new_reos_capture()
{
	ReOS_Capture *cap = malloc(sizeof(ReOS_Capture));
	cap->start = -1;
	cap->end = -1;
	cap->partial = 0;
	return cap;
}

/**
 * Allocates a new ReOS_Capture object and copies the provided object's value into
 * it.
 *
 * \param cap The object to copy
 * \return A new ReOS_Capture object identical to \a cap
 */
ReOS_Capture *reos_capture_clone(ReOS_Capture *cap)
{
	ReOS_Capture *clone = malloc(sizeof(ReOS_Capture));
	memcpy(clone, cap, sizeof(ReOS_Capture));
	return clone;
}

/**
 * Returns a write-safe copy of a ReOS_CaptureSet object.
 *
 * This function clones a match object if its reference count is greater than 1;
 * otherwise it increments its \c version number. This function is called at the
 * beginning of every function that can modify a ReOS_CaptureSet so that the object can be
 * safely shared between multiple ReOS_Thread objects.
 *
 * \param m The object to detach
 * \return A ReOS_CaptureSet object that is safe to write to, which could simply be \a set
 * if its reference count is 1.
 */
ReOS_CaptureSet *reos_captureset_detach(ReOS_CaptureSet *set)
{
	if (set->refs > 1) {
		ReOS_CaptureSet *clone = new_reos_captureset(set->free_list);

		if (set->captures) {
			clone->captures = new_reos_judylist((VoidPtrFunc)free_reos_compoundlist, 0, 0, 0);
			reos_judylist_iter_begin(ReOS_CompoundList, capture_list, set->captures) {
				reos_judylist_insert(clone->captures, iter_capture_list, reos_compoundlist_clone(capture_list));
				reos_judylist_iter_next(capture_list, set->captures);
			}
		}

		set->refs--;
		set = clone;
	}
	else
		set->version++;

	return set;
}

/**
 * Finds the ReOS_Capture object stored with \a capture_id and capture number \a
 * capture_num.
 *
 * If there is no capture list for \a i, one is created. If there is no
 * ReOS_Capture stored there under \a capture_id, one is also created.
 *
 * \param m The ReOS_CaptureSet from which to retrieve a ReOS_Capture or store a new
 * one
 * \param capture_id The id to retrieve
 * \param capture_num The number to retrieve
 * \return A new or previously stored ReOS_Capture object
 *
 * \todo Come up with a better name for this thing.
 */
static ReOS_Capture *reos_captureset_find_capture(ReOS_CaptureSet *set, int capture_num)
{
	if (!set->captures)
		set->captures = new_reos_judylist((VoidPtrFunc)free_reos_compoundlist, 0, 0, 0);

	reos_judylist_fail_get(set->captures, capture_num, ReOS_CompoundList, capture_list) {
		capture_list = new_reos_compoundlist(4, (VoidPtrFunc)free, (CloneFunc)reos_capture_clone);
		reos_judylist_insert(set->captures, capture_num, capture_list);
	}

	ReOS_Capture *cap;
	if (!reos_compoundlist_has_next(capture_list)) {
		cap = new_reos_capture();
		reos_compoundlist_push_tail(capture_list, cap);
	}
	else {
		// although the identities of the items in the list won't be modified,
		// their values will be, so we force a detach here
		reos_compoundlist_detach(capture_list);
		cap = reos_compoundlist_peek_tail(capture_list);
		if (cap->start != -1 && cap->end != -1) {
			cap = new_reos_capture();
			reos_compoundlist_push_tail(capture_list, cap);
		}
	}

	return cap;
}

ReOS_Capture *reos_captureset_get_capture(ReOS_CaptureSet *set, int capture_num)
{
	ReOS_CompoundList *capture_list;
	reos_judylist_get(set->captures, capture_num, capture_list);
	return reos_compoundlist_peek_tail(capture_list);
}

/**
 * Saves the starting index of a ReOS_Capture.
 *
 * ReOS_CaptureSet objects are shared and reference counted to minimize copying, so this
 * function first calls reos_captureset_detach() to ensure that we do not overwrite other
 * ReOS_Thread objects referencing this ReOS_CaptureSet. If the ReOS_CaptureSet's reference count is
 * greater than 1, a new ReOS_CaptureSet object is created and all its data and ReOS_Capture
 * objects copied over, then the object placed in the address \a set_ptr points
 * to. Otherwise, the ReOS_CaptureSet's \c version number is simply incremented and \a
 * set_ptr's value not modified.
 * 
 * Since a capture can be valid more than once per match, a \a capture_id is
 * needed to differentiate between copies. The first time a ReOS_Thread calls this
 * function or reos_captureset_save_end(), \c *capture_id is zero and a unique id is
 * assigned in its place. All subsequent calls by the ReOS_Thread to this function or
 * reos_captureset_save_end() must provide the same \a capture_id.
 *
 * \param set_ptr A pointer to the ReOS_CaptureSet object pointer, such that the latter can
 * be modified as copy-on-write requires
 * \param capture_id A pointer to the calling ReOS_Thread's \c capture_id
 * \param capture_num The capture number to save
 * \param index The input index to save as the start of the capture interval
 */
void reos_captureset_save_start(ReOS_CaptureSet **set_ptr, int capture_num, long index)
{
	*set_ptr = reos_captureset_detach(*set_ptr);
	ReOS_CaptureSet *set = *set_ptr;

	ReOS_Capture *cap = reos_captureset_find_capture(set, capture_num);
	cap->start = index;
	cap->partial = !cap->partial;
}

/**
 * Saves the ending index of a ReOS_Capture.
 *
 * \sa reos_captureset_save_start()
 *
 * \param set_ptr A pointer to the ReOS_CaptureSet object pointer, such that the latter can
 * be modified as copy-on-write requires
 * \param capture_id A pointer to the calling ReOS_Thread's \c capture_id
 * \param capture_num The capture number to save
 * \param index The input index to save as the start of the capture interval
 */
void reos_captureset_save_end(ReOS_CaptureSet **set_ptr, int capture_num, long index)
{
	*set_ptr = reos_captureset_detach(*set_ptr);
	ReOS_CaptureSet *set = *set_ptr;

	ReOS_Capture *cap = reos_captureset_find_capture(set, capture_num);
	cap->end = index;
	cap->partial = !cap->partial;
}

/**
 * Increments a ReOS_CaptureSet object's reference count.
 *
 * This function is called whenever a mutable reference to a ReOS_CaptureSet is made, or
 * simply to extend the lifetime of the object.
 *
 * \param m The ReOS_CaptureSet object to reference
 */
void reos_captureset_ref(ReOS_CaptureSet *set)
{
	set->refs++;
}

/**
 * Decrements a ReOS_CaptureSet object's reference count and possibly frees it.
 *
 * This function is called whenever a mutable reference to a ReOS_CaptureSet is destroyed.
 * If its reference count after dereferencing is 0, the object is freed.
 *
 * \param m The ReOS_CaptureSet object to dereference
 */
void reos_captureset_deref(ReOS_CaptureSet *set)
{
	if (--set->refs == 0)
		free_reos_captureset(set);
}

/**
 * Copies tokens from a capture interval into a buffer.
 *
 * This function can be used to easily transform a capture interval into the
 * actual tokens that were matched. If \a cap has no \c start index, tokens are
 * read from the beginning of input until the \c end index. If \a cap has no \c
 * end index, tokens are read from the \c start index until the end of input is
 * reached. \a buf must be large enough to hold the capture interval.
 *
 * \param cap The ReOS_Capture object whose interval to use
 * \param input The ReOS_Input object to read tokens from
 * \param buf The buffer to copy tokens into
 * \returns The number of tokens copied
 */
long reos_capture_reconstruct(ReOS_Capture *cap, ReOS_Input *input, void *buf)
{
	return input->indexed_read(buf, cap->end - cap->start, cap->start, input->data);
}

ReOS_BackrefBuffer *new_reos_backrefbuffer(ReOS_Capture *cap, ReOS_Input *input)
{
	ReOS_BackrefBuffer *b = malloc(sizeof(ReOS_BackrefBuffer));
	b->index = 0;
	b->length = cap->end - cap->start;
	b->tokens = malloc(sizeof(void *)*b->length);
	reos_capture_reconstruct(cap, input, b->tokens);

	return b;
}

void free_reos_backrefbuffer(ReOS_BackrefBuffer *b)
{
	if (b) {
		free(b->tokens);
		free(b);
	}
}
