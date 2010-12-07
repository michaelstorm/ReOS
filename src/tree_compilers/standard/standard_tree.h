#ifndef STANDARD_TREE_H
#define STANDARD_TREE_H

#include "reos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TreeNode TreeNode;
typedef void (*FreeTreeNodeFunc)(TreeNode *);
typedef void (*PrintTreeNodeFunc)(TreeNode *);
struct TreeNodeCompileFunctor;

typedef ReOS_Inst *(*ReOS_InstFactoryFunc)(int);
typedef int (*TreeNodeCompileFunc)(ReOS_Pattern *, int, TreeNode *, ReOS_InstFactoryFunc);

struct TreeNode
{
	int type;
	TreeNode *left;
	TreeNode *right;
	int x;
	int y;
	void *args;
};

enum
{
	NodeAlt = 200,
	NodeCat,
	NodeDot,
	NodeParen,
	NodeQuest,
	NodeStar,
	NodePlus,
	NodeBacktrack,
	NodeStart,
	NodeEnd,
	NodeRepCount,
	NodePosAhead,
	NodeNegAhead,
	NodeRecurse
};

void free_standard_tree_node(TreeNode *, FreeTreeNodeFunc);
void standard_tree_compile(ReOS_Pattern *, TreeNode *, ReOS_InstFactoryFunc, TreeNodeCompileFunc);
int standard_tree_node_compile(ReOS_Pattern *, int, TreeNode *, ReOS_InstFactoryFunc, TreeNodeCompileFunc);
void print_standard_tree(TreeNode *, PrintTreeNodeFunc);

#ifdef __cplusplus
}
#endif

#endif
