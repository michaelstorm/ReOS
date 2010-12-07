#include "reos_debugger.h"

void debugger_reset(ReOS_Debugger *d)
{
	d->start = 0;
	d->before_token = 0;
	d->before_inst = 0;
	d->after_inst = 0;
	d->after_token = 0;
	d->match = 0;
	d->failure = 0;
	d->end = 0;
}
