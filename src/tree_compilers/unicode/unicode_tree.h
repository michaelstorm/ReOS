#ifndef UNICODE_TREE_H
#define UNICODE_TREE_H

#include "standard_tree.h"
#include "unicode/utypes.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
	NodeUnicodeChar,
	NodeUnicodeRange
};

typedef struct UnicodeTreeNodeArgs UnicodeTreeNodeArgs;

struct UnicodeTreeNodeArgs
{
	UChar32 c1;
	UChar32 c2;
};

TreeNode *new_unicode_tree_node(int, TreeNode *, TreeNode *);
void free_unicode_tree_node(TreeNode *);
int unicode_tree_node_compile(ReOS_Pattern *, int, TreeNode *, ReOS_InstFactoryFunc);
void print_unicode_tree(TreeNode *);

#ifdef __cplusplus
}
#endif

#endif
