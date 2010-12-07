// Copyright 2007-2009 Russ Cox.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include "reos_pattern.h"
#include "unicode_inst.h"
#include "unicode_tree.h"
#include "unicode/uchar.h"
#include "unicode/ustdio.h"
#include "unicode/utypes.h"

TreeNode* new_unicode_tree_node(int type, TreeNode *left, TreeNode *right)
{
	TreeNode *node = malloc(sizeof(TreeNode));
	if (type == NodeUnicodeChar || type == NodeUnicodeRange)
		node->args = malloc(sizeof(UnicodeTreeNodeArgs));
	else
		node->args = 0;

	node->type = type;
	node->left = left;
	node->right = right;
	node->x = 0;
	node->y = 0;
	return node;
}

void free_unicode_tree_node(TreeNode *node)
{
	if (node) {
		if (node->type == NodeUnicodeChar || node->type == NodeUnicodeRange) {
			free(node->args);
			free_unicode_tree_node(node->left);
			free_unicode_tree_node(node->right);
			free(node);
		}
		else
			free_standard_tree_node(node, free_unicode_tree_node);
	}
}

int unicode_tree_node_compile(ReOS_Pattern *pattern, int index, TreeNode *node, ReOS_InstFactoryFunc inst_factory)
{
	switch (node->type) {
	case NodeUnicodeChar:
	{
		ReOS_Inst *lit = inst_factory(OpUnicodeChar);
		((UnicodeInstArgs *)lit->args)->c1 = ((UnicodeTreeNodeArgs *)node->args)->c1;
		pattern->set_inst(pattern, lit, index);
		return index+1;
	}

	case NodeUnicodeRange:
	{
		ReOS_Inst *range = inst_factory(OpUnicodeRange);
		UnicodeInstArgs *inst_args = range->args;
		UnicodeTreeNodeArgs *node_args = node->args;
		inst_args->c1 = node_args->c1;
		inst_args->c2 = node_args->c2;
		pattern->set_inst(pattern, range, index);
		return index+1;
	}

	default:
		return standard_tree_node_compile(pattern, index, node, inst_factory, unicode_tree_node_compile);
	}
}

void print_unicode_tree(TreeNode *node)
{
	UnicodeTreeNodeArgs *args = (UnicodeTreeNodeArgs *)node->args;

	switch (node->type) {
	case NodeUnicodeChar:
	{
		printf("Token(");
		if (u_isgraph(args->c1)) {
			printf("'");
			UFILE *u_stdout = u_finit(stdout, 0, 0);
			u_fputc(args->c1, u_stdout);
			u_fclose(u_stdout);
			printf("'");
		}
		else
			printf("0x%x", args->c1);
		printf(")");
		break;
	}

	case NodeUnicodeRange:
	{
		printf("Range(");

		UFILE *u_stdout = u_finit(stdout, 0, 0);
		if (u_isgraph(args->c1)) {
			printf("'");
			u_fputc(args->c1, u_stdout);
			printf("'");
		}
		else
			printf("0x%x", args->c1);

		printf("-");

		if (u_isgraph(args->c2)) {
			printf("'");
			u_fputc(args->c2, u_stdout);
			printf("'");
		}
		else
			printf("0x%x", args->c2);
		u_fclose(u_stdout);

		printf(")");
		break;
	}

	default:
		print_standard_tree(node, print_unicode_tree);
	}
}
