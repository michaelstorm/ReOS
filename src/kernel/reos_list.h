#ifndef REOS_LIST_H
#define REOS_LIST_H

#include <Judy.h>

#include "judy_macros.h"
#include "reos_types.h"

#define reos_simplelist_has_next(l) ((l)->head)

#define reos_compoundlist_has_next(l) \
	((l)->impl->tail->pos < (l)->impl->tail->len \
		   || ((l)->impl->tail->len == 0 \
			   && (l)->impl->tail->prev \
			   && (l)->impl->tail->prev->pos < (l)->impl->tail->prev->len))

#define reos_compoundlist_detach(l) \
	if ((l)->impl->refs > 1) { \
		(l)->impl->refs--; \
		(l)->impl = reos_compoundlistimpl_clone((l)->impl); \
	}

#define foreach_simple_body(type, data, list, iter) \
	type *data; \
	ReOS_SimpleListIter iter; \
	reos_simplelistiter_init(&iter, list); \
	while (reos_simplelistiter_has_next(&iter) ? ((data = (type *)reos_simplelistiter_get_next(&iter)), 1) : 0)

#define foreach_compound_body(type, data, list, iter) \
	type *data; \
	ReOS_CompoundListIter iter; \
	reos_compoundlistiter_init(&iter, list); \
	while (reos_compoundlistiter_has_next(&iter) ? ((data = (type *)reos_compoundlistiter_get_next(&iter)), 1) : 0)

#define foreach_index_body(type, data, list, iter) \
	type *data; \
	IndexListIter iter; \
	indexlistiter_init(&iter, list); \
	while (indexlistiter_has_next(&iter) ? ((data = (type *)indexlistiter_get_next(&iter)), 1) : 0)

#define foreach_simple(type, data, list) foreach_simple_body(type, data, (list), iter_##data)
#define foreach_compound(type, data, list) foreach_compound_body(type, data, (list), iter_##data)
#define foreach_index(type, data, list) foreach_index_body(type, data, (list), iter_##data)

#ifdef __cplusplus
extern "C" {
#endif

ReOS_SimpleList *new_reos_simplelist(VoidPtrFunc);
void free_reos_simplelist(ReOS_SimpleList *);
ReOS_SimpleList *reos_simplelist_clone(ReOS_SimpleList *, CloneFunc);
void reos_simplelist_dump(ReOS_SimpleList *);

void reos_simplelist_push_head(ReOS_SimpleList *, void *);
void reos_simplelist_push_tail(ReOS_SimpleList *, void *);
void *reos_simplelist_pop_head(ReOS_SimpleList *);
void *reos_simplelist_pop_tail(ReOS_SimpleList *);
void *reos_simplelist_peek_head(ReOS_SimpleList *);
void *reos_simplelist_peek_tail(ReOS_SimpleList *);

void reos_simplelistiter_init(ReOS_SimpleListIter *, ReOS_SimpleList *);
int reos_simplelistiter_has_next_func(ReOS_SimpleListIter *);
void *reos_simplelistiter_get_next(ReOS_SimpleListIter *);
void *reos_simplelistiter_peek_next(ReOS_SimpleListIter *);

ReOS_CompoundList *new_reos_compoundlist(int, VoidPtrFunc, CloneFunc);
int free_reos_compoundlist(ReOS_CompoundList *);
ReOS_CompoundList *reos_compoundlist_clone(ReOS_CompoundList *);
ReOS_CompoundList *reos_compoundlist_clone_with_func(ReOS_CompoundList *, CloneFunc);
void reos_compoundlist_unshare(ReOS_CompoundList *);
void reos_compoundlist_dump(ReOS_CompoundList *);

// exported because it's used in compoundlist_detach
ReOS_CompoundListImpl *reos_compoundlistimpl_clone(ReOS_CompoundListImpl *);

long reos_compoundlist_length(ReOS_CompoundList *);
void reos_compoundlist_push_tail(ReOS_CompoundList *, void *);
void reos_compoundlist_push_head(ReOS_CompoundList *, void *);
void reos_compoundlist_insert_from_head(ReOS_CompoundList *, int, void *);
void *reos_compoundlist_pop_head(ReOS_CompoundList *);
void *reos_compoundlist_pop_tail(ReOS_CompoundList *);
void *reos_compoundlist_peek_head(ReOS_CompoundList *);
void *reos_compoundlist_peek_tail(ReOS_CompoundList *);

void reos_compoundlistiter_init(ReOS_CompoundListIter *, ReOS_CompoundList *);
int reos_compoundlistiter_has_next_func(ReOS_CompoundListIter *);
void *reos_compoundlistiter_get_next(ReOS_CompoundListIter *);
void *reos_compoundlistiter_peek_next(ReOS_CompoundListIter *);

ReOS_JudyList *new_reos_judylist(VoidPtrFunc, VoidPtrFunc, CloneFunc, VoidPtrFunc);
int free_reos_judylist(ReOS_JudyList *);
void free_judy(void *, VoidPtrFunc);

void reos_judylist_ref_elements(ReOS_JudyList *);
void reos_judylist_deref_elements(ReOS_JudyList *);
ReOS_JudyList *reos_judylist_clone(ReOS_JudyList *);
// exported because it's used in the macros above
ReOS_JudyListImpl *reos_judylistimpl_clone(ReOS_JudyListImpl *);

void reos_judylist_push_tail(ReOS_JudyList *, void *);
void *reos_judylist_pop_tail(ReOS_JudyList *);

#ifdef __cplusplus
}
#endif

#endif
