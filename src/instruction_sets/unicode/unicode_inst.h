#ifndef UNICODE_INST_H
#define UNICODE_INST_H

#include "reos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
	OpUnicodeChar,
	OpUnicodeRange
};

typedef struct UnicodeInstArgs UnicodeInstArgs;

struct UnicodeInstArgs
{
	char c1;
	char c2;
};

ReOS_Inst *unicode_inst_factory(int);
int execute_unicode_inst(ReOS_Kernel *, ReOS_Thread *, ReOS_Inst *, int);
void print_unicode_inst(ReOS_Inst *);

#ifdef __cplusplus
}
#endif

#endif
