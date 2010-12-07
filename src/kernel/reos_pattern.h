#ifndef REOS_PATTERN_H
#define REOS_PATTERN_H

#include "reos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

ReOS_Pattern *new_mem_pattern();
void free_mem_pattern(ReOS_Pattern *);

void print_pattern(ReOS_Kernel *);

#ifdef __cplusplus
}
#endif

#endif
