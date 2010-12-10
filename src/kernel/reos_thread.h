#ifndef REOS_THREAD_H
#define REOS_THREAD_H

#include "reos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

ReOS_Thread *new_reos_thread(ReOS_CompoundList *, int);
void free_reos_thread(ReOS_Thread *);
void delete_reos_thread(ReOS_Thread *);
ReOS_Thread *reos_thread_clone(ReOS_Thread *);

ReOS_ThreadList *new_reos_threadlist(int);
void free_reos_threadlist(ReOS_ThreadList *);

void reos_threadlist_push_head(ReOS_ThreadList *, ReOS_Thread *, int);
void reos_threadlist_push_tail(ReOS_ThreadList *, ReOS_Thread *, int);
ReOS_Thread *reos_threadlist_pop_head(ReOS_ThreadList *);

ReOS_Branch *new_reos_branch(int);
ReOS_Branch *reos_branch_strong_ref(ReOS_Branch *);
ReOS_Branch *reos_branch_weak_ref(ReOS_Branch *);
void reos_branch_strong_deref(ReOS_Branch *);
void reos_branch_weak_deref(ReOS_Branch *);
void reos_branch_print(ReOS_Branch *);
int reos_branch_alive(ReOS_Branch *);
int reos_branch_succeeded(ReOS_Branch *);

#ifdef __cplusplus
}
#endif

#endif
