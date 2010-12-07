// Copyright 2007-2009 Russ Cox.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef ASCII_TREE_H
#define ASCII_TREE_H

#include "standard_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
	NodeAsciiChar,
	NodeAsciiRange
};

typedef struct AsciiTreeNodeArgs AsciiTreeNodeArgs;

struct AsciiTreeNodeArgs
{
	char c1;
	char c2;
};

TreeNode *new_ascii_tree_node(int, TreeNode *, TreeNode *);
void free_ascii_tree_node(TreeNode *);
int ascii_tree_node_compile(ReOS_Pattern *, int, TreeNode *, ReOS_InstFactoryFunc);
void print_ascii_tree(TreeNode *);

#ifdef __cplusplus
}
#endif

#endif
