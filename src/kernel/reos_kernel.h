#ifndef REOS_KERNEL_H
#define REOS_KERNEL_H

#include "reos_buffer.h"
#include "reos_capture.h"
#include "reos_list.h"
#include "reos_pattern.h"
#include "reos_thread.h"

#define REOS_ANCHORED 0x1
#define REOS_BACKTRACK_MATCHING 0x2
#define REOS_PARTIAL 0x4

#ifdef __cplusplus
extern "C" {
#endif

ReOS_Kernel *new_reos_kernel(ReOS_Pattern *, ExecuteInstFunc, int);
void free_reos_kernel(ReOS_Kernel *);
void reos_kernel_clear_memory_pools(ReOS_Kernel *);
void reos_kernel_process_inst_ret(ReOS_Kernel *, ReOS_Thread *, int);
int reos_kernel_execute(ReOS_Kernel *, ReOS_Input *, int, int);
void reos_kernel_bootstrap(ReOS_Kernel *, int);
void reos_kernel_push_current_threadlist(ReOS_Kernel *, ReOS_Thread *, int);
void reos_kernel_push_next_threadlist(ReOS_Kernel *, ReOS_Thread *, int);

#ifdef __cplusplus
}
#endif

#endif
