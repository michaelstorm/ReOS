// Copyright 2007-2009 Russ Cox.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef STRING_TREE_H
#define STRING_TREE_H

#include "standard_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
	OpString
};

enum
{
	NodeString
};

TreeNode *new_string_tree_node(int type, TreeNode *left, TreeNode *right);
void free_string_tree_node(TreeNode *);
int string_tree_node_compile(ReOS_Pattern *, int, TreeNode *, ReOS_InstFactoryFunc);
void print_string_tree(TreeNode *);

#ifdef __cplusplus
}
#endif

#endif
