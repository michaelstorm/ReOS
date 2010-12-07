#include <stdio.h>
#include <stdlib.h>
#include "ascii_inst.h"
#include "ascii_tree.h"

TreeNode* new_ascii_tree_node(int type, TreeNode *left, TreeNode *right)
{
	TreeNode *node = malloc(sizeof(TreeNode));
	if (type == NodeAsciiChar || type == NodeAsciiRange)
		node->args = malloc(sizeof(AsciiTreeNodeArgs));
	else
		node->args = 0;

	node->type = type;
	node->left = left;
	node->right = right;
	node->x = 0;
	node->y = 0;
	return node;
}

void free_ascii_tree_node(TreeNode *node)
{
	if (node) {
		if (node->type == NodeAsciiChar || node->type == NodeAsciiRange) {
			free(node->args);
			free_ascii_tree_node(node->left);
			free_ascii_tree_node(node->right);
			free(node);
		}
		else
			free_standard_tree_node(node, free_ascii_tree_node);
	}
}

int ascii_tree_node_compile(ReOS_Pattern *pattern, int index, TreeNode *node, ReOS_InstFactoryFunc inst_factory)
{
	switch (node->type) {
	case NodeAsciiChar:
	{
		ReOS_Inst *lit = inst_factory(OpAsciiChar);
		((AsciiInstArgs *)lit->args)->c1 = ((AsciiTreeNodeArgs *)node->args)->c1;
		pattern->set_inst(pattern, lit, index);
		return index+1;
	}

	case NodeAsciiRange:
	{
		ReOS_Inst *range = inst_factory(OpAsciiRange);
		AsciiInstArgs *inst_args = range->args;
		AsciiTreeNodeArgs *node_args = node->args;
		inst_args->c1 = node_args->c1;
		inst_args->c2 = node_args->c2;
		pattern->set_inst(pattern, range, index);
		return index+1;
	}

	default:
		return standard_tree_node_compile(pattern, index, node, ascii_inst_factory, ascii_tree_node_compile);
	}
}

void print_ascii_tree(TreeNode *node)
{
	switch (node->type) {
	default:
		print_standard_tree(node, print_ascii_tree);
		return;

	case NodeAsciiChar:
		printf("Char('%c')", ((AsciiTreeNodeArgs *)node->args)->c1);
		break;

	case NodeAsciiRange:
	{
		AsciiTreeNodeArgs *args = (AsciiTreeNodeArgs *)node->args;
		printf("Range('%c'-'%c')", args->c1, args->c2);
		break;
	}
	}
}
