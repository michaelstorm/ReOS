#ifndef PERCENT_DEBUGGER_H
#define PERCENT_DEBUGGER_H

#include "reos_debugger.h"

#ifdef __cplusplus
extern "C" {
#endif

ReOS_Debugger *new_percent_debugger();
void free_percent_debugger(ReOS_Debugger *);
void percent_debugger_print_status(ReOS_Debugger *, ReOS_Kernel *);

#ifdef __cplusplus
}
#endif

#endif
