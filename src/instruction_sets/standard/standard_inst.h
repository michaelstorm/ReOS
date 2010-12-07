#ifndef STANDARD_INST_H
#define STANDARD_INST_H

#include "reos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StandardInstArgs StandardInstArgs;

enum
{
	OpMatch = 50,
	OpJmp,
	OpSplit,
	OpAny,
	OpSaveStart,
	OpSaveEnd,
	OpBacktrack,
	OpStart,
	OpEnd,
	OpBranch,
	OpNegBranch,
	OpRecurse
};

struct StandardInstArgs
{
	int x;
	int y;
};

ReOS_Inst *standard_inst_factory(int);
int execute_standard_inst(ReOS_Kernel *, ReOS_Thread *, ReOS_Inst *, int);
void print_standard_inst(ReOS_Inst *);

#ifdef __cplusplus
}
#endif

#endif
