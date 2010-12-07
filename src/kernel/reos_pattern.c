#include <Judy.h>
#include "reos_pattern.h"
#include "reos_list.h"
#include "reos_stdlib.h"

void free_reos_inst(ReOS_Inst *inst)
{
	free(inst->args);
	free(inst);
}

ReOS_Inst *get_mem_inst(ReOS_Pattern *pattern, int index)
{
	ReOS_Inst *inst;
	judy_get(pattern->data, index, inst);
	return inst;
}

void set_mem_inst(ReOS_Pattern *pattern, ReOS_Inst *inst, int index)
{
	judy_insert(pattern->data, index, inst);
}

ReOS_Pattern *new_mem_pattern()
{
	ReOS_Pattern *a = malloc(sizeof(ReOS_Pattern));
	a->data = 0;

	a->get_inst = get_mem_inst;
	a->set_inst = set_mem_inst;
	return a;
}

void free_mem_pattern(ReOS_Pattern *a)
{
	if (a) {
		free_judy(a->data, (VoidPtrFunc)free_reos_inst);
		free(a);
	}
}
