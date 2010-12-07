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
int reos_branch_alive(ReOS_Branch *);
int reos_branch_succeeded(ReOS_Branch *);

ReOS_JoinRoot *new_reos_joinroot(ReOS_Branch *);
void free_reos_joinroot(ReOS_JoinRoot *);
ReOS_JoinRoot *joinroot_clone(ReOS_JoinRoot *);
ReOS_Branch *joinroot_get_root_branch(ReOS_JoinRoot *);

#ifdef __cplusplus
}
#endif

#endif
