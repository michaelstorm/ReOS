#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "standard_inst.h"
#include "string_tree.h"

TreeNode* new_string_tree_node(int type, TreeNode *left, TreeNode *right)
{
	TreeNode *node = malloc(sizeof(TreeNode));
	if (type == NodeString)
		node->args = malloc(sizeof(char)*64);
	else
		node->args = 0;

	node->type = type;
	node->left = left;
	node->right = right;
	node->x = 0;
	node->y = 0;
	return node;
}

void free_string_tree_node(TreeNode *node)
{
	if (node) {
		if (node->type == NodeString) {
			free(node->args);
			free_string_tree_node(node->left);
			free_string_tree_node(node->right);
			free(node);
		}
		else
			free_standard_tree_node(node, free_string_tree_node);
	}
}

int string_tree_node_compile(ReOS_Pattern *pattern, int index, TreeNode *node, ReOS_InstFactoryFunc inst_factory)
{
	if (node->type == NodeString) {
		ReOS_Inst *rec = inst_factory(OpString);
		strcpy(rec->args, node->args);
		pattern->set_inst(pattern, rec, index);
		return index+1;
	}
	else
		return standard_tree_node_compile(pattern, index, node, inst_factory, string_tree_node_compile);
}

void print_string_tree(TreeNode *node)
{
	if (node->type == NodeString)
		printf("String(\"%s\")", (char *)node->args);
	else
		print_standard_tree(node, print_string_tree);
}
