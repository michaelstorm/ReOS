#ifndef SHELL_DEBUGGER_H
#define SHELL_DEBUGGER_H

#include "reos_debugger.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ShellDebuggerData ShellDebuggerData;
typedef struct ShellDebuggerCommand ShellDebuggerCommand;
typedef struct ShellDebuggerOption ShellDebuggerOption;
typedef struct ShellBreakCondition ShellBreakCondition;
typedef int (*ShellDebuggerCommandFunc)(ReOS_Debugger *, ShellDebuggerCommand *, ReOS_Kernel *);

struct ShellDebuggerData
{
	ShellDebuggerCommand *last_command;
	void *break_debuggers;
	int break_num;
	int display;
	int cont;

	PrintInstFunc print_inst;
	PrintInputFunc print_input;
};

struct ShellDebuggerCommand
{
	char *name;
	ReOS_SimpleList *options;
};

struct ShellBreakCondition
{
	ReOS_Debugger *shell_debugger;
	int num;
	int enabled;
	int deleted;
	int line;
	int opcode;
	int index;
};

struct ShellDebuggerOption
{
	char *key_name;
	int value_is_num;

	union
	{
		int value_num;
		char *value_str;
	};
};

ReOS_Debugger *new_shell_debugger();
void free_shell_debugger(ReOS_Debugger *);
void free_shell_debugger_command(ShellDebuggerCommand *);
void free_shell_debugger_option(ShellDebuggerOption *);

void shell_debugger_prompt(ReOS_Debugger *, ReOS_Kernel *);
ShellDebuggerCommand *parse_shell_command(char *);
void shell_debugger_print_state(ShellDebuggerData *, ReOS_Kernel *);
void shell_debugger_print_threadlist(ShellDebuggerData *, ReOS_Kernel *, ReOS_ThreadList *, int);
void shell_debugger_print_thread(ShellDebuggerData *, ReOS_Kernel *, ReOS_Thread *);
void shell_debugger_print_pattern(ShellDebuggerData *, ReOS_Kernel *);

void breakpoint_debugger_check_break(ReOS_Debugger *, ReOS_Kernel *);

#ifdef __cplusplus
}
#endif

#endif
