#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "ascii_inst.h"
#include "reos_kernel.h"
#include "standard_inst.h"

ReOS_Inst *ascii_inst_factory(int opcode)
{
	if (opcode == OpAsciiChar || opcode == OpAsciiRange) {
		ReOS_Inst *inst = malloc(sizeof(ReOS_Inst));
		inst->opcode = opcode;
		inst->args = malloc(sizeof(AsciiInstArgs));
		return inst;
	}
	else
		return standard_inst_factory(opcode);
}

void print_ascii_inst(ReOS_Inst *inst)
{
	AsciiInstArgs *args = inst->args;
	if (inst->opcode == OpAsciiChar)
		printf("char %c", args->c1);
	else if (inst->opcode == OpAsciiRange)
		printf("range %c, %c", args->c1, args->c2);
	else
		print_standard_inst(inst);
}

int ascii_test_backref(ReOS_Kernel *k, void *current_token, void *ref_token)
{
	if (!current_token || !ref_token)
		return 0;

	char current = *(char *)current_token;
	char ref = *(char *)ref_token;
	return current == ref;
}

int execute_ascii_inst(ReOS_Kernel *k, ReOS_Thread *thread, ReOS_Inst *inst, int ops)
{
	AsciiInstArgs *args = inst->args;
	char c;
	if (k->current_token)
		c = *(char *)k->current_token;

	if (inst->opcode == OpAsciiChar) {
		if (k->current_token == 0) {
			if (ops & REOS_PARTIAL)
				return ReOS_InstRetMatch;
			else
				return ReOS_InstRetDrop;
		}

		if (args->c1 != -1) {
			if (c == args->c1)
				return ReOS_InstRetConsume;
			else
				return ReOS_InstRetDrop;
		}
		else {
			switch (args->c2) {
			case 'w':
				if (isalnum(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			case 'W':
				if (!isalnum(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			case 's':
				if (isspace(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			case 'S':
				if (!isspace(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			case 'd':
				if (isdigit(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			case 'D':
				if (!isdigit(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			case 'v':
				if (isspace(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			case 'V':
				if (!isspace(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			case 'h':
				if (isspace(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			case 'H':
				if (!isspace(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			default:
				fprintf(stderr, "error: bad special\n");
				return ReOS_InstRetHalt;
			}
		}
	} else if (inst->opcode == OpAsciiRange) {
		if (k->current_token == 0)
			return ReOS_InstRetDrop;

		if ((c >= args->c1) && (c <= args->c2))
			return ReOS_InstRetConsume;
		else
			return ReOS_InstRetDrop;
	}
	else
		return execute_standard_inst(k, thread, inst, ops);
}
