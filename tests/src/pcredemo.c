/*************************************************
*           PCRE DEMONSTRATION PROGRAM           *
*************************************************/

/* This is a demonstration program to illustrate the most straightforward ways
of calling the PCRE regular expression library from a C program. See the
pcresample documentation for a short discussion ("man pcresample" if you have
the PCRE man pages installed).

In Unix-like environments, if PCRE is installed in your standard system
libraries, you should be able to compile this program using this command:

gcc -Wall pcredemo.c -lpcre -o pcredemo

If PCRE is not installed in a standard place, it is likely to be installed with
support for the pkg-config mechanism. If you have pkg-config, you can compile
this program using this command:

gcc -Wall pcredemo.c `pkg-config --cflags --libs libpcre` -o pcredemo

If you do not have pkg-config, you may have to use this:

gcc -Wall pcredemo.c -I/usr/local/include -L/usr/local/lib \
  -R/usr/local/lib -lpcre -o pcredemo

Replace "/usr/local/include" and "/usr/local/lib" with wherever the include and
library files for PCRE are installed on your system. Only some operating
systems (e.g. Solaris) use the -R option.

Building under Windows:

If you want to statically link this program against a non-dll .a file, you must
define PCRE_STATIC before including pcre.h, otherwise the pcre_malloc() and
pcre_free() exported functions will be declared __declspec(dllimport), with
unwanted results. So in this environment, uncomment the following line. */

/* #define PCRE_STATIC */

#include <stdio.h>
#include <string.h>
#include <pcre.h>

#include "ascii_expression.h"
#include "ascii_input.h"
#include "ascii_inst.h"
#include "ascii_tree.h"
#include "pikevm_kernel.h"
#include "shell_debugger.h"

#define OVECCOUNT 30    /* should be a multiple of 3 */

int main(int argc, char **argv)
{
	printf("pcre:\n");

	pcre *re;
	pcre_extra *pe;
	const char *error;
	char *pattern;
	char *subject;
	int erroffset;
	int find_all;
	int ovector[OVECCOUNT];
	int subject_length;
	int rc, i;

	find_all = 0;
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-g") == 0) find_all = 1;
			else break;
	}

	if (argc - i != 2) {
		printf("Two arguments required: a regex and a subject string\n");
		return 1;
	}

	pattern = argv[i];
	subject = argv[i+1];
	subject_length = (int)strlen(subject);

	re = pcre_compile(
		pattern,              /* the pattern */
		0,                    /* default options */
		&error,               /* for error message */
		&erroffset,           /* for error offset */
		NULL);                /* use default character tables */

	if (re == NULL) {
		printf("PCRE compilation failed at offset %d: %s\n", erroffset, error);
		return 1;
	}

	pe = pcre_study(
		re,             /* result of pcre_compile() */
		0,              /* no options exist */
		&error);        /* set to NULL or points to a message */

	rc = pcre_exec(
		re,                   /* the compiled pattern */
		pe,                   /* extra data */
		subject,              /* the subject string */
		subject_length,       /* the length of the subject */
		0,                    /* start at offset 0 in the subject */
		0,                    /* default options */
		ovector,              /* output vector for substring information */
		OVECCOUNT);           /* number of elements in the output vector */

	if (rc < 0) {
		switch(rc) {
			case PCRE_ERROR_NOMATCH: printf("No match\n"); break;
			default: printf("Matching error %d\n", rc); break;
		}
		pcre_free(re);
	}
	else {
		printf("\nMatch succeeded at offset %d\n", ovector[0]);

		if (rc == 0) {
			rc = OVECCOUNT/3;
			printf("ovector only has room for %d captured substrings\n", rc - 1);
		}

		for (i = 0; i < rc; i++) {
			char *substring_start = subject + ovector[2*i];
			int substring_length = ovector[2*i+1] - ovector[2*i];
			printf("%2d: %.*s\n", i, substring_length, substring_start);
		}

		if (!find_all)
			pcre_free(re);
		else {
			for (;;) {
				int options = 0;                 /* Normally no options */
				int start_offset = ovector[1];   /* Start at end of previous match */

				if (ovector[0] == ovector[1]) {
					if (ovector[0] == subject_length)
						break;

					options = PCRE_NOTEMPTY_ATSTART | PCRE_ANCHORED;
				}

				rc = pcre_exec(
					re,                   /* the compiled pattern */
					pe,                   /* extra data */
					subject,              /* the subject string */
					subject_length,       /* the length of the subject */
					start_offset,         /* starting offset in the subject */
					options,              /* options */
					ovector,              /* output vector for substring information */
					OVECCOUNT);           /* number of elements in the output vector */

				if (rc == PCRE_ERROR_NOMATCH) {
					if (options == 0)
						break;

					ovector[1] = start_offset + 1;
					continue;
				}

				if (rc < 0) {
					printf("Matching error %d\n", rc);
					pcre_free(re);
					return 1;
				}

				printf("\nMatch succeeded again at offset %d\n", ovector[0]);

				if (rc == 0) {
					rc = OVECCOUNT/3;
					printf("ovector only has room for %d captured substrings\n", rc - 1);
				}

				for (i = 0; i < rc; i++) {
					char *substring_start = subject + ovector[2*i];
					int substring_length = ovector[2*i+1] - ovector[2*i];
					printf("%2d: %.*s\n", i, substring_length, substring_start);
				}

			}

			printf("\n");
			pcre_free(re);
		}
	}

	printf("pikevm:\n");

	TreeNode *tree = ascii_expression_compile(pattern);
	if (!tree) {
		fprintf(stderr, "Error: Invalid regular expression\n\n");
		exit(1);
	}

	Input *input;
	input = new_ascii_string_input(subject);

	Pattern *pikevm_pattern = new_mem_pattern();
	standard_tree_compile(pikevm_pattern, tree, ascii_inst_factory, ascii_tree_node_compile);
	free_ascii_tree_node(tree);

	PikeVm *vm = new_pikevm(pikevm_pattern, execute_ascii_inst, -1);
	pikevm_execute(vm, input, 0, 0);

	printf("Match\tSub\tIndexes\tReconstruction\n");

	int match_num = 0;
	foreach_simple(Match, match, vm->matches) {
		printf("%d", match_num++);

		judy_iter_begin(Submatch, sub, match->submatches) {
			printf("\t%ld\t[%ld-", iter_sub, sub->start);
			if (sub->end >= 0) {
				int match_len = sub->end - sub->start;

				char reconst[match_len];
				match_reconstruct(match, vm->token_buf->input, reconst);
				printf("%ld]\t%.*s", sub->end, match_len, reconst);
			}
			printf("\n");

			judy_iter_next(sub, match->submatches);
		}

		if (!match->submatches)
			printf("\n");
	}

	free_ascii_string_input(input);
	free_mem_pattern(pikevm_pattern);
	free_pikevm(vm);
	return 0;
}

/* End of pcredemo.c */
