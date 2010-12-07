// Copyright 2007-2009 Russ Cox.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pikevm_kernel.h"
#include "profile_debugger.h"
#include "shell_debugger.h"
#include "unicode_input.h"
#include "unicode_inst.h"
#include "unicode_expression.h"
#include "unicode_tree.h"
#include "unicode/ustdio.h"

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
			"	-u, --utf8\n"
			"		treat INPUT encoding as UTF-8; conflicts with -c\n"
			"	-c, --codepage CODEPAGE\n"
			"		convert INPUT encoding from CODEPAGE to Unicode; can only be used with -f\n"
			"	-b, --backtrack-subs\n"
			"		find all submatches by sometimes backtracking\n");
	exit(0);
}

int main(int argc, char **argv)
{
	int ops = 0;
	int matches = 0;
	int debug = 0;
	int profile = 0;
	int file = 0;
	int offset = 0;
	char *codepage = 0;

	int i;
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-m") || !strcmp(argv[i], "--matches"))
			matches = 1;
		else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--debug"))
			debug = 1;
		else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--profile"))
			profile = 1;
		else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
			print_usage();
		else if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--regex")) {
			i++;
			break;
		}
		else if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--offset"))
			offset = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--start"))
			ops |= PIKEVM_ANCHORED;
		else if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--file"))
			file = 1;
		else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--codepage"))
			codepage = argv[++i];
		else if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--backtrack-matching"))
			ops |= PIKEVM_BACKTRACK_MATCHING;
		else
			break;
	}

	if (codepage && !file) {
		fprintf(stderr, "Error: Codepage can only be specified for files\n\n");
		print_usage();
	}
	else if (i+2 < argc) {
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

	TreeNode *tree = unicode_expression_compile(argv[i++]);
	if (!tree) {
		fprintf(stderr, "Error: Invalid regular expression\n\n");
		print_usage();
	}
	
	if (debug) {
		print_unicode_tree(tree);
		printf("\n");
	}

	Input *input;
	if (file) {
		if (codepage)
			input = new_unicode_file_input(argv[i], codepage);
		else
			input = new_unicode_file_input(argv[i], "UTF-8");
	}
	else
		input = new_unicode_string_input(argv[i]);

	Pattern *pattern = new_mem_pattern();
	standard_tree_compile(pattern, tree, unicode_inst_factory, unicode_tree_node_compile);
	free_unicode_tree_node(tree);

	PikeVm *vm = new_pikevm(pattern, execute_unicode_inst, -1);

	Debugger *shell_debugger;
	if (debug) {
		shell_debugger = new_shell_debugger();
		ShellDebuggerData *data = shell_debugger->data;
		data->print_inst = print_unicode_inst;
		if (!file)
			data->print_input = print_unicode_string_input;

		simplelist_push_tail(vm->debuggers, shell_debugger);
	}

	Debugger *profile_debugger;
	if (profile) {
		profile_debugger = new_profile_debugger();
		simplelist_push_tail(vm->debuggers, profile_debugger);
	}

	pikevm_execute(vm, input, offset, ops);

	if (matches) {
		printf("| Match\tSub\tId\tIndexes\tReconstruction\n");
		printf("+ -----\n");

		int match_num = 0;
		foreach_simple(Match, match, vm->matches) {
			printf("| %d\t\t\t[%ld-", match_num++, match->start);
			if (match->end >= 0) {
				int match_len = match->end - match->start;

				char reconst[match_len];
				match_reconstruct(match, vm->token_buf->input, reconst);
				printf("%ld]\t%.*s", match->end, match_len, reconst);
			}
			printf("\n");

			if (match->submatches) {
				judylist_iter_begin(JudyList, sub_list, match->submatches) {
					printf("|\t| %ld\t\n", iter_sub_list);
					printf("|\t|\t+ -----\n");

					judylist_iter_begin(Submatch, sub, sub_list) {
						printf("|\t|\t| %ld\t[%ld-", iter_sub, sub->start);

						if (sub->end >= 0) {
							int submatch_len = sub->end - sub->start;

							UChar32 reconst[submatch_len];
							submatch_reconstruct(sub, vm->token_buf->input, reconst);

							printf("%ld]\t", sub->end);

							UFILE *u_stdout = u_finit(stdout, 0, 0);
							int char_index;
							for (char_index = 0; char_index < submatch_len; char_index++)
								u_fputc(reconst[char_index], u_stdout);
							u_fclose(u_stdout);
						}
						printf("\n|\t|\t+ -----\n");

						judylist_iter_next(sub, sub_list);
					}
					printf("|\t+ -----\n");

					judylist_iter_next(sub_list, match->submatches);
				}
			}
		}
		printf("+ -----\n");
	}

	if (profile)
		profile_debugger_print_results(profile_debugger);

	if (debug)
		free_shell_debugger(shell_debugger);
	if (profile)
		free_profile_debugger(profile_debugger);

	if (file)
		free_unicode_file_input(input);
	else
		free_unicode_string_input(input);

	free_mem_pattern(pattern);
	free_pikevm(vm);
	return 0;
}
