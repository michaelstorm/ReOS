#ifndef ASCII_INST_H
#define ASCII_INST_H

#include "reos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
	OpAsciiChar,
	OpAsciiRange
};

typedef struct AsciiInstArgs AsciiInstArgs;

struct AsciiInstArgs
{
	char c1;
	char c2;
};

ReOS_Inst *ascii_inst_factory(int);
int ascii_test_backref(ReOS_Kernel *, void *, void *);
int execute_ascii_inst(ReOS_Kernel *, ReOS_Thread *, ReOS_Inst *, int);
void print_ascii_inst(ReOS_Inst *);

#ifdef __cplusplus
}
#endif

#endif
