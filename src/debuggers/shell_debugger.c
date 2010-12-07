#include <antlr3.h>
#include <assert.h>
#include <Judy.h>
#include <stdlib.h>
#include "reos_kernel.h"
#include "shell_debugger.h"
#include "shell_debugLexer.h"
#include "shell_debugParser.h"

#define DISPLAY_STATE 0x1
#define DISPLAY_INPUT 0x2

ShellBreakCondition *new_breakcondition(ReOS_Debugger *d)
{
	ShellBreakCondition *cond = malloc(sizeof(ShellBreakCondition));
	cond->shell_debugger = d;
	cond->enabled = 1;
	cond->deleted = 0;
	cond->index = -1;
	cond->line = -1;
	cond->opcode = -1;
	return cond;
}

void free_breakpoint_debugger(ReOS_Debugger *d)
{
	free(d->data);
	free(d);
}

ReOS_Debugger *new_shell_debugger(ReOS_Kernel *k)
{
	ReOS_Debugger *d = calloc(1, sizeof(ReOS_Debugger));
	d->before_inst = shell_debugger_prompt;
	d->data = malloc(sizeof(ShellDebuggerData));

	ShellDebuggerData *data = d->data;
	data->last_command = 0;
	data->break_debuggers = 0;
	data->break_num = 0;
	data->display = 0;
	data->cont = 0;
	data->print_inst = 0;
	data->print_input = 0;
	return d;
}

void free_shell_debugger(ReOS_Debugger *d)
{
	if (d) {
		ShellDebuggerData *data = d->data;
		free_shell_debugger_command(data->last_command);
		free_judy(data->break_debuggers, (VoidPtrFunc)free_breakpoint_debugger);

		free(data);
		free(d);
	}
}

void free_shell_debugger_command(ShellDebuggerCommand *command)
{
	if (command) {
		free_reos_simplelist(command->options);
		free(command->name);
		free(command);
	}
}

void free_shell_debugger_option(ShellDebuggerOption *option)
{
	if (option) {
		free(option->key_name);
		if (!option->value_is_num)
			free(option->value_str);
		free(option);
	}
}

static void print_break(ShellBreakCondition *cond)
{
	int comma = 0;
	if (cond->line != -1) {
		if (comma)
			printf(", ");
		printf("line=%d", cond->line);
		comma = 1;
	}

	if (cond->index != -1) {
		if (comma)
			printf(", ");
		printf("index=%d", cond->index);
		comma = 1;
	}

	if (cond->opcode != -1) {
		if (comma)
			printf(", ");
		printf("opcode=%d", cond->opcode);
		comma = 1;
	}
}

int shell_debugger_step_token(ReOS_Debugger *debugger, ShellDebuggerCommand *command, ReOS_Kernel *k)
{
	debugger_reset(debugger);
	debugger->before_token = shell_debugger_prompt;
	return 1;
}

int shell_debugger_step_instruction(ReOS_Debugger *debugger, ShellDebuggerCommand *command, ReOS_Kernel *k)
{
	debugger_reset(debugger);
	debugger->before_inst = shell_debugger_prompt;
	return 1;
}

int shell_debugger_print(ReOS_Debugger *debugger, ShellDebuggerCommand *command, ReOS_Kernel *k)
{
	ShellDebuggerData *data = debugger->data;
	if (!reos_simplelist_has_next(command->options)) {
		printf("Specify an option\n");
		return 0;
	}

	ShellDebuggerOption *option = reos_simplelist_peek_head(command->options);
	if (!strcmp(option->value_str, "state"))
		shell_debugger_print_state(data, k);
	else if (!strcmp(option->value_str, "pattern"))
		shell_debugger_print_pattern(data, k);
	else if (!strcmp(option->value_str, "input")) {
		if (data->print_input)
			data->print_input(k);
		else
			printf("No input display function has been provided\n");
	}
	return 0;
}

int shell_debugger_display(ReOS_Debugger *debugger, ShellDebuggerCommand *command, ReOS_Kernel *k)
{
	ShellDebuggerData *data = debugger->data;
	if (!reos_simplelist_has_next(command->options)) {
		printf("ReOS_State:\n");
		shell_debugger_print_state(data, k);
		data->display |= DISPLAY_STATE;
		
		if (data->print_input) {
			printf("\nInput:\n");
			data->print_input(k);
			data->display |= DISPLAY_INPUT;
		}

		return 0;
	}

	ShellDebuggerOption *option = reos_simplelist_peek_head(command->options);
	if (!strcmp(option->value_str, "state")) {
		shell_debugger_print_state(data, k);
		data->display |= DISPLAY_STATE;
	}
	else if (!strcmp(option->value_str, "input")) {
		if (data->print_input) {
			data->print_input(k);
			data->display |= DISPLAY_INPUT;
		}
		else
			printf("No input display function has been provided\n");
	}
	return 0;
}

int shell_debugger_undisplay(ReOS_Debugger *debugger, ShellDebuggerCommand *command, ReOS_Kernel *k)
{
	ShellDebuggerData *data = debugger->data;
	if (!reos_simplelist_has_next(command->options)) {
		data->display = 0;
		return 0;
	}

	ShellDebuggerOption *option = reos_simplelist_peek_head(command->options);
	if (!strcmp(option->value_str, "state")) {
		if (data->display & DISPLAY_STATE)
			data->display ^= DISPLAY_STATE;
	}
	else if (!strcmp(option->value_str, "input")) {
		if (data->display & DISPLAY_INPUT)
			data->display ^= DISPLAY_INPUT;
	}
	return 0;
}

int shell_debugger_dump(ReOS_Debugger *debugger, ShellDebuggerCommand *command, ReOS_Kernel *k)
{
	if (!reos_simplelist_has_next(command->options)) {
		printf("Specify an option\n");
		return 0;
	}

	ShellDebuggerOption *option = reos_simplelist_peek_head(command->options);
	if (!strcmp(option->value_str, "current"))
		reos_compoundlist_dump(k->state.current_thread_list->list);
	else if (!strcmp(option->value_str, "next"))
		reos_compoundlist_dump(k->state.next_thread_list->list);
	return 0;
}

int shell_debugger_break(ReOS_Debugger *debugger, ShellDebuggerCommand *command, ReOS_Kernel *k)
{
	ShellDebuggerData *data = debugger->data;
	if (!reos_simplelist_has_next(command->options)) {
		printf("Specify one or more break conditions\n");
		return 0;
	}

	ShellBreakCondition *cond = new_breakcondition(debugger);
	foreach_simple(ShellDebuggerOption, option, command->options) {
		if (!option->key_name || !strcmp(option->key_name, "line"))
			cond->line = option->value_num;
		else if (!strcmp(option->key_name, "index"))
			cond->index = option->value_num;
		else if (!strcmp(option->key_name, "opcode"))
			cond->opcode = option->value_num;
		else
			printf("Unrecognized option: \"%s\"\n", option->key_name);
	}

	ReOS_Debugger *break_dbg = malloc(sizeof(ReOS_Debugger));
	debugger_reset(break_dbg);
	break_dbg->before_inst = breakpoint_debugger_check_break;
	break_dbg->data = cond;

	cond->num = data->break_num++;
	printf("Breakpoint %d set at ", cond->num+1);
	print_break(cond);
	printf("\n");

	judy_insert(data->break_debuggers, cond->num, break_dbg);
	reos_simplelist_push_tail(k->debuggers, break_dbg);
	return 0;
}

int shell_debugger_info(ReOS_Debugger *debugger, ShellDebuggerCommand *command, ReOS_Kernel *k)
{
	ShellDebuggerData *data = debugger->data;
	if (!reos_simplelist_has_next(command->options)) {
		printf("Specify an option\n");
		return 0;
	}

	ShellDebuggerOption *option = reos_simplelist_pop_head(command->options);
	if (!strcmp(option->value_str, "break")) {
		printf("Num\tEnb\tConditions\n");

		ShellBreakCondition *cond;
		judy_iter_begin(ReOS_Debugger, d, data->break_debuggers) {
			cond = d->data;
			if (!cond->deleted) {
				printf("%d\t%c\t", cond->num+1, cond->enabled ? 'y' : 'n');
				print_break(cond);
				printf("\n");
			}

			judy_iter_next(d, data->break_debuggers);
		}
	}
	else
		printf("Unrecognized option: \"%s\"\n", option->value_str);
	return 0;
}

int shell_debugger_delete(ReOS_Debugger *debugger, ShellDebuggerCommand *command, ReOS_Kernel *k)
{
	ShellDebuggerData *data = debugger->data;
	if (!reos_simplelist_has_next(command->options)) {
		ShellBreakCondition *cond;
		judy_iter_begin(ReOS_Debugger, d, data->break_debuggers) {
			cond = d->data;
			cond->deleted = 1;
			judy_iter_next(d, data->break_debuggers);
		}
	}
	else {
		ReOS_Debugger *d;
		ShellBreakCondition *cond;

		foreach_simple(ShellDebuggerOption, option, command->options) {
			judy_get(data->break_debuggers, option->value_num-1, d);
			cond = d->data;
			cond->deleted = 1;
		}
	}
	return 0;
}

int shell_debugger_enable(ReOS_Debugger *debugger, ShellDebuggerCommand *command, ReOS_Kernel *k)
{
	ShellDebuggerData *data = debugger->data;
	if (!reos_simplelist_has_next(command->options)) {
		ShellBreakCondition *cond;
		judy_iter_begin(ReOS_Debugger, d, data->break_debuggers) {
			cond = d->data;
			cond->enabled = 1;
			judy_iter_next(d, data->break_debuggers);
		}
	}
	else {
		ReOS_Debugger *d;
		ShellBreakCondition *cond;

		foreach_simple(ShellDebuggerOption, option, command->options) {
			judy_get(data->break_debuggers, option->value_num-1, d);
			cond = d->data;
			cond->enabled = 1;
		}
	}
	return 0;
}

int shell_debugger_disable(ReOS_Debugger *debugger, ShellDebuggerCommand *command, ReOS_Kernel *k)
{
	ShellDebuggerData *data = debugger->data;
	if (!reos_simplelist_has_next(command->options)) {
		ShellBreakCondition *cond;
		judy_iter_begin(ReOS_Debugger, d, data->break_debuggers) {
			cond = d->data;
			cond->enabled = 0;
			judy_iter_next(d, data->break_debuggers);
		}
	}
	else {
		ReOS_Debugger *d;
		ShellBreakCondition *cond;

		foreach_simple(ShellDebuggerOption, option, command->options) {
			judy_get(data->break_debuggers, option->value_num-1, d);
			cond = d->data;
			cond->enabled = 0;
		}
	}
	return 0;
}

int shell_debugger_jump(ReOS_Debugger *debugger, ShellDebuggerCommand *command, ReOS_Kernel *k)
{
	ShellDebuggerData *data = debugger->data;
	if (!reos_simplelist_has_next(command->options)) {
		printf("Specify a line to jump to\n");
		return 0;
	}

	ShellDebuggerOption *option = reos_simplelist_pop_head(command->options);
	ReOS_Thread *thread = new_reos_thread(k->free_thread_list, option->value_num);
	thread->capture_set = new_reos_captureset(k->free_captureset_list);

	reos_threadlist_push_head(k->state.current_thread_list, thread, 1);

	printf("%d.\t", thread->pc);
	shell_debugger_print_thread(data, k, thread);
	printf("\n");
	return 0;
}

int shell_debugger_continue(ReOS_Debugger *debugger, ShellDebuggerCommand *command, ReOS_Kernel *k)
{
	ShellDebuggerData *data = debugger->data;
	data->cont = 1;
	return 1;
}

int shell_debugger_quit(ReOS_Debugger *debugger, ShellDebuggerCommand *command, ReOS_Kernel *k)
{
	exit(0);
	return 0;
}

static void *command_table[][2] =
{
	{"st",        shell_debugger_step_token},
	{"si",        shell_debugger_step_instruction},
	{"n",         shell_debugger_step_instruction},
	{"p",         shell_debugger_print},
	{"print",     shell_debugger_print},
	{"d",         shell_debugger_display},
	{"display",   shell_debugger_display},
	{"u",         shell_debugger_undisplay},
	{"undisplay", shell_debugger_undisplay},
	{"dump",      shell_debugger_dump},
	{"b",         shell_debugger_break},
	{"break",     shell_debugger_break},
	{"i",         shell_debugger_info},
	{"info",      shell_debugger_info},
	{"delete",    shell_debugger_delete},
	{"enable",    shell_debugger_enable},
	{"disable",   shell_debugger_disable},
	{"j",         shell_debugger_jump},
	{"jmp",       shell_debugger_jump},
	{"jump",      shell_debugger_jump},
	{"c",         shell_debugger_continue},
	{"cont",      shell_debugger_continue},
	{"continue",  shell_debugger_continue},
	{"q",         shell_debugger_quit},
	{"quit",      shell_debugger_quit},
	{0, 0}
};

void shell_debugger_prompt(ReOS_Debugger *debugger, ReOS_Kernel *k)
{
	char line[128];
	int done = 0;
	ShellDebuggerData *data = debugger->data;

	if (data->display & DISPLAY_STATE) {
		printf("ReOS_State:\n");
		shell_debugger_print_state(data, k);
	}
	if (data->display & DISPLAY_INPUT) {
		printf("\nInput:\n");
		data->print_input(k);
	}

	if (data->cont)
		return;

	ReOS_Thread *thread;
	if (reos_compoundlist_has_next(k->state.current_thread_list->list)) {
		thread = reos_compoundlist_peek_head(k->state.current_thread_list->list);
		printf("%d.\t", thread->pc);
		shell_debugger_print_thread(data, k, thread);
	}
	else {
		if (reos_compoundlist_has_next(k->state.next_thread_list->list)) {
			thread = reos_compoundlist_peek_head(k->state.next_thread_list->list);
			printf("%d.\t", thread->pc);
			shell_debugger_print_thread(data, k, thread);
		}
		else
			printf("no instruction");
	}
	printf("\n");

	while (!done) {
		printf("(reos) ");
		fgets(line, 128, stdin);

		ShellDebuggerCommand *command;
		if (strlen(line) > 1) {
			free_shell_debugger_command(data->last_command);
			command = parse_shell_command(line);
			data->last_command = command;
		}
		else {
			command = data->last_command;
			if (!command)
				continue;
		}

		int i;
		for (i = 0; command_table[i][0]; i++) {
			if (!strcmp(command->name, command_table[i][0])) {
				done = ((ShellDebuggerCommandFunc)command_table[i][1])(debugger, command, k);
				break;
			}
		}

		if (!command_table[i][0])
			printf("Unrecognized command: \"%s\"\n", command->name);
	}
}

void shell_debugger_print_state(ShellDebuggerData *debugger_data, ReOS_Kernel *k)
{
	printf("Index: %ld\n", k->sp);

	/**
	  * \todo Create DISPLAY_MATCHES and give it its own option.
	  */
	printf("Matches: ");
	foreach_simple(ReOS_CaptureSet, capture_set, k->matches) {
		printf("%p", capture_set);
		if (capture_set->captures) {
			printf(": {");
			reos_judylist_iter_begin(ReOS_Capture, cap, capture_set->captures) {
				printf("[%ld-", cap->start);
				if (cap->end >= 0)
					printf("%ld", cap->end);
				printf("], ");
				reos_judylist_iter_next(cap, capture_set->captures);
			}
			printf("}");
		}
		printf(", ");
	}
	printf("\n");

	printf("%p\n", k->state.next_thread_list->list->impl);

	if (debugger_data->print_inst) {
		printf("ReOS_Threads:\n");
		printf("\tCurrent:\n");
		shell_debugger_print_threadlist(debugger_data, k, k->state.current_thread_list, 1);

		ReOS_CompoundListNode *next_list_node = k->state.next_thread_list->list->impl->head;
		if (next_list_node && next_list_node->len > 0) {
			printf("\n\tNext:\n");
			shell_debugger_print_threadlist(debugger_data, k, k->state.next_thread_list, 0);
		}
	}
}

static void print_branch_tree(ReOS_Branch *b, int indent)
{
	int i;
	for (i = 0; i < indent; i++)
		printf("\t");

	if (b->negated)
		printf("(!)");

	if (b->matched)
		printf("%p:m\n", b);
	else
		printf("%p:%d\n", b, b->num_threads);

	if (reos_simplelist_has_next(b->and_children)) {
		for (i = 0; i < indent; i++)
			printf("\t");
		printf("\tand_children:\n");

		foreach_simple(ReOS_Branch, child, b->and_children)
			print_branch_tree(child, indent+1);
	}

	if (reos_simplelist_has_next(b->or_children)) {
		for (i = 0; i < indent; i++)
			printf("\t");
		printf("\tor_children:\n");

		foreach_simple(ReOS_Branch, child, b->or_children)
			print_branch_tree(child, indent+1);
	}
}

void shell_debugger_print_threadlist(ShellDebuggerData *debugger_data, ReOS_Kernel *k, ReOS_ThreadList *thread_list, int carat)
{
	ReOS_CompoundListIter iter;
	reos_compoundlistiter_init(&iter, thread_list->list);

	ReOS_Thread *carat_thread = reos_compoundlist_has_next(thread_list->list) ? reos_compoundlist_peek_head(thread_list->list) : 0;
	while (reos_compoundlistiter_has_next(&iter)) {
		ReOS_Thread *thread = reos_compoundlistiter_get_next(&iter);

		if (carat && carat_thread && carat_thread->pc == thread->pc)
			printf("\t> %d. ", thread->pc);
		else
			printf("\t  %d. ", thread->pc);

		shell_debugger_print_thread(debugger_data, k, thread);
		printf("\n");

		if (thread->join_root && joinroot_get_root_branch(thread->join_root))
			print_branch_tree(joinroot_get_root_branch(thread->join_root), 2);
	}
}

void shell_debugger_print_thread(ShellDebuggerData *debugger_data, ReOS_Kernel *k, ReOS_Thread *thread)
{
	ReOS_Inst *inst = k->pattern->get_inst(k->pattern, thread->pc);
	if (debugger_data->print_inst)
		debugger_data->print_inst(inst);

	if (thread->deps || thread->refs) {
		printf(" deps: {");
		if (thread->deps) {
			foreach_compound(ReOS_Branch, branch, thread->deps) {
				if (branch->negated)
					printf("(!)");

				if (branch->matched)
					printf("%p:m, ", branch);
				else
					printf("%p:%d, ", branch, branch->num_threads);
			}
		}

		printf("} refs: {");
		if (thread->refs) {
			foreach_compound(ReOS_Branch, branch, thread->refs) {
				if (branch->negated)
					printf("(!)");

				if (branch->matched)
					printf("%p:m, ", branch);
				else
					printf("%p:%d, ", branch, branch->num_threads);
			}
		}

		printf("}");
	}

	ReOS_CaptureSet *capture_set = thread->capture_set;
	if (capture_set->captures) {
		if_judylist_is_not_empty(capture_set->captures)
			printf(" | ");

		reos_judylist_iter_begin(ReOS_CompoundList, capture_list, capture_set->captures) {
			printf("%ld: { ", iter_capture_list);
			foreach_compound(ReOS_Capture, cap, capture_list) {
				printf("[");
				if (cap->start >= 0)
					printf("%ld", cap->start);
				printf("-");
				if (cap->end >= 0)
					printf("%ld", cap->end);

				printf("], ");
			}
			printf("}, ");

			reos_judylist_iter_next(capture_list, capture_set->captures);
		}
	}
}

void shell_debugger_print_pattern(ShellDebuggerData *debugger_data, ReOS_Kernel *k)
{
	int i = 0;
	judy_iter_begin(ReOS_Inst, inst, k->pattern->data) {
		printf("%d. ", i++);
		debugger_data->print_inst(inst);
		printf("\n");

		judy_iter_next(inst, k->pattern->data);
	}
}

ShellDebuggerCommand *parse_shell_command(char *string)
{
	pANTLR3_INPUT_STREAM input = antlr3NewAsciiStringCopyStream((pANTLR3_UINT8)string, strlen(string), 0);
	pshell_debugLexer lex = shell_debugLexerNew(input);
	pANTLR3_COMMON_TOKEN_STREAM tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lex));
	pshell_debugParser parser = shell_debugParserNew(tokens);

	ShellDebuggerCommand *command = parser->line(parser);

	parser->free(parser);
	tokens->free(tokens);
	lex->free(lex);
	input->close(input);
	return command;
}

void breakpoint_debugger_check_break(ReOS_Debugger *debugger, ReOS_Kernel *k)
{
	ReOS_Thread *next_thread = reos_compoundlist_peek_head(k->state.current_thread_list->list);
	ShellBreakCondition *cond = debugger->data;

	if (cond->deleted || !cond->enabled)
		return;

	if (cond->line != -1 && next_thread->pc != cond->line)
		return;

	if (cond->opcode != -1 && k->pattern->get_inst(k->pattern, next_thread->pc)->opcode != cond->opcode)
		return;

	if (cond->index != -1 && k->sp != cond->index)
		return;

	printf("Breakpoint %d at ", cond->num);
	print_break(cond);
	printf("\n");

	shell_debugger_prompt(cond->shell_debugger, k);
}
