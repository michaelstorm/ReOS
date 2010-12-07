#include <stdio.h>
#include <stdlib.h>
#include "percent_debugger.h"
#include "reos_kernel.h"

ReOS_Debugger *new_percent_debugger()
{
	ReOS_Debugger *d = malloc(sizeof(ReOS_Debugger));
	debugger_reset(d);
	d->after_token = percent_debugger_print_status;
	return d;
}

void free_percent_debugger(ReOS_Debugger *d)
{
	free(d->data);
	free(d);
}

void percent_debugger_print_status(ReOS_Debugger *debugger, ReOS_Kernel *k)
{
	printf("%ld\n", k->sp);
}
