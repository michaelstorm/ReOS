#include <stdio.h>
#include <stdlib.h>
#include "reos_kernel.h"
#include "standard_inst.h"
#include "unicode_inst.h"
#include "unicode/uchar.h"
#include "unicode/ustdio.h"

ReOS_Inst *unicode_inst_factory(int opcode)
{
	if (opcode == OpUnicodeChar || opcode == OpUnicodeRange) {
		ReOS_Inst *inst = malloc(sizeof(ReOS_Inst));
		inst->opcode = opcode;
		inst->args = malloc(sizeof(UnicodeInstArgs));
		return inst;
	}
	else
		return standard_inst_factory(opcode);
}

void print_unicode_inst(ReOS_Inst *inst)
{
	UnicodeInstArgs *args = inst->args;
	UFILE *u_stdout = u_finit(stdout, 0, 0);

	switch (inst->opcode) {
	case OpUnicodeChar:
		printf("char ");
		u_fputc(args->c1, u_stdout);
		break;

	case OpUnicodeRange:
		printf("range ");
		u_fputc(args->c1, u_stdout);
		printf(", ");
		u_fputc(args->c2, u_stdout);
		break;

	default:
		print_standard_inst(inst);
	}

	u_fclose(u_stdout);
}

int execute_unicode_inst(ReOS_Kernel *k, ReOS_Thread *thread, ReOS_Inst *inst, int ops)
{
	UnicodeInstArgs *args = inst->args;
	UChar32 c;
	if (k->current_token)
		c = *(UChar32 *)k->current_token;

	switch (inst->opcode) {
	case OpUnicodeChar:
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
				if (u_isalnum(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			case 'W':
				if (!u_isalnum(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			case 's':
				if (u_isspace(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			case 'S':
				if (!u_isspace(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			case 'd':
				if (u_isdigit(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			case 'D':
				if (!u_isdigit(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			case 'v':
				if (u_isspace(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			case 'V':
				if (!u_isspace(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			case 'h':
				if (u_isspace(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			case 'H':
				if (!u_isspace(c))
					return ReOS_InstRetConsume;
				else
					return ReOS_InstRetDrop;

			default:
				fprintf(stderr, "error: bad special\n");
				return ReOS_InstRetHalt;
			}
		}

	case OpUnicodeRange:
		if (k->current_token == 0)
			return ReOS_InstRetDrop;

		if ((c >= args->c1) && (c <= args->c2))
			return ReOS_InstRetConsume;
		else
			return ReOS_InstRetDrop;

	default:
		return execute_standard_inst(k, thread, inst, ops);
	}
}
