// Copyright 2007-2009 Russ Cox.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <Judy.h>
#include <stdio.h>
#include <string.h>

#include "ascii_expression.h"
#include "ascii_input.h"
#include "ascii_inst.h"
#include "ascii_tree.h"
#include "reos_debugger.h"
#include "reos_kernel.h"
#include "reos_stdlib.h"
#include "percent_debugger.h"
#include "profile_debugger.h"
#include "shell_debugger.h"

static void print_usage()
{
	fprintf(stderr,
			"Usage: ascii [OPTION]... REGEX INPUT\n"
			"Options:\n"
			"	-m, --matches\n"
			"		show matches\n"
			"	-d, --debug\n"
			"		attach GDB-like shell debugger\n"
			"	-p, --profile\n"
			"		collect performance data\n"
			"	-h, --help\n"
			"		display this help\n"
			"	-r, --regex\n"
			"		force the next argument to be interpreted as REGEX and not as an option\n"
			"	-o, --offset OFFSET\n"
			"		start matching at index OFFSET of input\n"
			"	-a, --anchored\n"
			"		match only at the starting offset\n"
			"	-f, --file\n"
			"		treat INPUT as a file path\n"
			"	-b, --backtrack-caps\n"
			"		find all captures by sometimes backtracking\n");
	exit(0);
}

int main(int argc, char **argv)
{
	setvbuf(stdout, NULL, _IOLBF, 0);

	int ops = 0;
	int matches = 0;
	int debug = 0;
	int profile = 0;
	int file = 0;
	int offset = 0;
	int percent = 0;

	int i;
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-m") || !strcmp(argv[i], "--matches"))
			matches = 1;
		else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--debug"))
			debug = 1;
		else if (!strcmp(argv[i], "--profile"))
			profile = 1;
		else if (!strcmp(argv[i], "--percent"))
			percent = 1;
		else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
			print_usage();
		else if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--regex")) {
			i++;
			break;
		}
		else if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--offset"))
			offset = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-a") || !strcmp(argv[i], "--anchored"))
			ops |= REOS_ANCHORED;
		else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--partial"))
			ops |= REOS_PARTIAL;
		else if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--file"))
			file = 1;
		else if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--backtrack-matching"))
			ops |= REOS_BACKTRACK_MATCHING;
		else
			break;
	}

	if (i+2 < argc) {
		fprintf(stderr, "Error: Too many arguments\n\n");
		print_usage();
	}
	else if (i+1 > argc) {
		fprintf(stderr, "Error: Specify a regex and an input\n\n");
		print_usage();
	}
	else if (i+2 > argc) {
		fprintf(stderr, "Error: Specify an input\n\n");
		print_usage();
	}

	TreeNode *tree = ascii_expression_compile(argv[i++]);
	if (!tree) {
		fprintf(stderr, "Error: Invalid regular expression\n\n");
		print_usage();
	}

	if (debug) {
		print_ascii_tree(tree);
		printf("\n");
	}

	ReOS_Input *input;
	if (file)
		input = new_ascii_file_input(argv[i]);
	else
		input = new_ascii_string_input(argv[i]);

	ReOS_Pattern *pattern = new_mem_pattern();
	standard_tree_compile(pattern, tree, ascii_inst_factory, ascii_tree_node_compile);
	free_ascii_tree_node(tree);

	ReOS_Kernel *vm = new_reos_kernel(pattern, execute_ascii_inst, -1);
	vm->test_backref = ascii_test_backref;

	ReOS_Debugger *shell_debugger;
	if (debug) {
		shell_debugger = new_shell_debugger();
		ShellDebuggerData *data = shell_debugger->data;
		data->print_inst = print_ascii_inst;
		if (!file)
			data->print_input = print_ascii_string_input;

		reos_simplelist_push_tail(vm->debuggers, shell_debugger);
	}

	ReOS_Debugger *profile_debugger;
	if (profile) {
		profile_debugger = new_profile_debugger();
		reos_simplelist_push_tail(vm->debuggers, profile_debugger);
	}

	ReOS_Debugger *percent_debugger;
	if (percent) {
		percent_debugger = new_percent_debugger();
		reos_simplelist_push_tail(vm->debuggers, percent_debugger);
	}

	reos_kernel_execute(vm, input, offset, ops);

	if (matches) {
		printf("| Match\tSub\tIndexes\tReconstruction\n");
		printf("+ -----\n");

		int match_num = 0;
		foreach_simple(ReOS_CaptureSet, capture_set, vm->matches) {
			printf("| %d\n", match_num++);

			if (capture_set->captures) {
				printf("|\t+ -----\n");
				reos_judylist_iter_begin(ReOS_CompoundList, capture_list, capture_set->captures) {
					printf("|\t| %ld\n", iter_capture_list);
					foreach_compound(ReOS_Capture, cap, capture_list) {
						printf("|\t|\t[%ld-", cap->start);

						if (cap->end >= 0) {
							int capture_len = cap->end - cap->start;

							char reconst[capture_len];
							reos_capture_reconstruct(cap, vm->token_buf->input, reconst);
							printf("%ld]\t%.*s", cap->end, capture_len, reconst);
						}
						printf("\n");
					}
					printf("|\t+ -----\n");

					reos_judylist_iter_next(capture_list, capture_set->captures);
				}
			}
			printf("+ -----\n");
		}
	}

	if (profile)
		profile_debugger_print_results(profile_debugger);

	if (debug)
		free_shell_debugger(shell_debugger);
	if (profile)
		free_profile_debugger(profile_debugger);

	if (file)
		free_ascii_file_input(input);
	else
		free_ascii_string_input(input);

	free_mem_pattern(pattern);
	free_reos_kernel(vm);
	return 0;
}
