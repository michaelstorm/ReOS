#include <stdio.h>
#include <stdlib.h>
#include "standard_inst.h"
#include "standard_tree.h"

void free_standard_tree_node(TreeNode *node, FreeTreeNodeFunc free_func)
{
	if (node) {
		free_func(node->left);
		free_func(node->right);
		free(node);
	}
}

void standard_tree_compile(ReOS_Pattern *pattern, TreeNode *tree,
						   ReOS_InstFactoryFunc inst_factory,
						   TreeNodeCompileFunc tree_node_compile)
{
	ReOS_Inst *match = inst_factory(OpMatch);
	int end = tree_node_compile(pattern, 0, tree, inst_factory);
	pattern->set_inst(pattern, match, end);
}

int standard_tree_node_compile(ReOS_Pattern *pattern, int index, TreeNode *node,
							   ReOS_InstFactoryFunc inst_factory,
							   TreeNodeCompileFunc tree_node_compile)
{
	switch (node->type) {
	default:
		perror("bad standard_tree_node_compile");

	/*
	  e1|e2:
				split L1, L2
			L1: codes for e1
				jmp L3
			L2: codes for e2
			L3:
	*/
	case NodeAlt:
	{
		ReOS_Inst *split_inst = inst_factory(OpSplit);
		StandardInstArgs *split_inst_args =
				(StandardInstArgs *)split_inst->args;

		split_inst_args->x = index+1;
		split_inst_args->y =
				tree_node_compile(pattern, split_inst_args->x, node->left,
								  inst_factory) + 1;

		pattern->set_inst(pattern, split_inst, index);

		ReOS_Inst *jmp_inst = inst_factory(OpJmp);
		((StandardInstArgs *)jmp_inst->args)->x =
				tree_node_compile(pattern, split_inst_args->y, node->right,
								  inst_factory);

		pattern->set_inst(pattern, jmp_inst, split_inst_args->y - 1);

		return ((StandardInstArgs *)jmp_inst->args)->x;
	}

	/*
		e1e2:
			codes for e1
			codes for e2
	*/
	case NodeCat:
		return tree_node_compile(pattern,
								 tree_node_compile(pattern, index, node->left,
												   inst_factory),
								 node->right, inst_factory);

	case NodeDot:
	{
		ReOS_Inst *dot = inst_factory(OpAny);
		pattern->set_inst(pattern, dot, index);
		return index+1;
	}

	/*
		(e):
			save 2*k
			e
			save 2*k+1
	*/
	case NodeParen:
	{
		ReOS_Inst *left_paren = inst_factory(OpSaveStart);
		((StandardInstArgs *)left_paren->args)->x = node->x;
		pattern->set_inst(pattern, left_paren, index);

		ReOS_Inst *right_paren = inst_factory(OpSaveEnd);
		((StandardInstArgs *)right_paren->args)->x = node->x;

		int next_index = tree_node_compile(pattern, index+1, node->left,
										   inst_factory);
		pattern->set_inst(pattern, right_paren, next_index);
		return next_index+1;
	}

	/*
		e?:
				split L1, L2
			L1: codes for e
			L2:

		e??:
				split L2, L1
			L1: codes for e
			L2:
	*/
	case NodeQuest:
	{
		ReOS_Inst *split_inst = inst_factory(OpSplit);
		int next = tree_node_compile(pattern, index+1, node->left,
									 inst_factory);
		StandardInstArgs *split_inst_args =
				(StandardInstArgs *)split_inst->args;

		if (node->x)
		{
			split_inst_args->x = next;
			split_inst_args->y = index+1;
		}
		else
		{
			split_inst_args->x = index+1;
			split_inst_args->y = next;
		}

		pattern->set_inst(pattern, split_inst, index);
		return next;
	}

	/*
		e*:
			L1: split L2, L3
			L2: codes for e
				jmp L1
			L3:

		e*?:
			L1: split L3, L2
			L2: codes for e
				jmp L1
			L3:
	*/
	case NodeStar:
	{
		int next = tree_node_compile(pattern, index+1, node->left,
									 inst_factory);
		ReOS_Inst *jmp_inst = inst_factory(OpJmp);
		((StandardInstArgs *)jmp_inst->args)->x = index;
		pattern->set_inst(pattern, jmp_inst, next);

		ReOS_Inst *split_inst = inst_factory(OpSplit);
		StandardInstArgs *split_inst_args =
				(StandardInstArgs *)split_inst->args;

		if (node->x)
		{
			split_inst_args->x = next+1;
			split_inst_args->y = index+1;
		}
		else
		{
			split_inst_args->x = index+1;
			split_inst_args->y = next+1;
		}

		pattern->set_inst(pattern, split_inst, index);
		return next+1;
	}

	/*
		e+:
			L1: codes for e
				split L1, L2
			L2:

		e+?:
			L1: codes for e
				split L2, L1
			L2:
	*/
	case NodePlus:
	{
		int next = tree_node_compile(pattern, index, node->left, inst_factory);
		ReOS_Inst *split_inst = inst_factory(OpSplit);
		StandardInstArgs *split_inst_args =
				(StandardInstArgs *)split_inst->args;

		if (node->x)
		{
			split_inst_args->x = next+1;
			split_inst_args->y = index;
		}
		else
		{
			split_inst_args->x = index;
			split_inst_args->y = next+1;
		}
		pattern->set_inst(pattern, split_inst, next);
		return next+1;
	}

	case NodeBacktrack:
	{
		ReOS_Inst *backtrack = inst_factory(OpBacktrack);
		((StandardInstArgs *)backtrack->args)->x = node->x;
		pattern->set_inst(pattern, backtrack, index);
		return index+1;
	}

	case NodeStart:
	{
		ReOS_Inst *start = inst_factory(OpStart);
		pattern->set_inst(pattern, start, index);
		return index+1;
	}

	case NodeEnd:
	{
		ReOS_Inst *end = inst_factory(OpEnd);
		pattern->set_inst(pattern, end, index);
		return index+1;
	}

	/*
		e{rep}:
			(L1: codes for e) x rep

		e{min,-1}:
			(L1: codes for e) x min
			L2: split L3, L4
			L3: codes for e
				jmp L2
			L4:

		e{-1,max}:
			(L1: split L2, L3
			 L2: codes for e) x max
			L3:

		e{min,max}:
			(L1: codes for e) x min
			(L2: split L3, L4
			 L3: codes for e) x max
			L4:
	*/
	case NodeRepCount:
	{
		int	next = index;

		int i;
		for (i = 0; i < node->x; i++)
			next = tree_node_compile(pattern, next, node->left, inst_factory);

		if (node->y == -1) {
			TreeNode star = {NodeStar, node->left, 0, 0, 0, 0};
			return tree_node_compile(pattern, next, &star, inst_factory);
		}
		else {
			TreeNode quest = {NodeQuest, node->left, 0, 0, 0, 0};
			for (i = node->x; i < node->y; i++)
				next = tree_node_compile(pattern, next, &quest, inst_factory);
			return next;
		}
	}

	/*
		(?=e1):
			branch L1, L2
			L1: codes for e1
				match
			L2:
	*/
	case NodePosAhead:
	{
		int next = tree_node_compile(pattern, index+1, node->left,
									 inst_factory);
		ReOS_Inst *match_inst = inst_factory(OpMatch);
		pattern->set_inst(pattern, match_inst, next);

		ReOS_Inst *branch_inst = inst_factory(OpBranch);
		StandardInstArgs *branch_inst_args = (StandardInstArgs *)branch_inst->args;
		branch_inst_args->x = index+1;
		branch_inst_args->y = next+1;
		pattern->set_inst(pattern, branch_inst, index);
		return next+1;
	}

	/*
		(?!e1):
			neg-branch L1, L2
			L1: codes for e1
				match
			L2:
	*/
	case NodeNegAhead:
	{
		int next = tree_node_compile(pattern, index+1, node->left,
									 inst_factory);
		ReOS_Inst *match_inst = inst_factory(OpMatch);
		pattern->set_inst(pattern, match_inst, next);

		ReOS_Inst *negbranch_inst = inst_factory(OpNegBranch);
		StandardInstArgs *negbranch_inst_args =
				(StandardInstArgs *)negbranch_inst->args;
		negbranch_inst_args->x = index+1;
		negbranch_inst_args->y = next+1;
		pattern->set_inst(pattern, negbranch_inst, index);
		return next+1;
	}

	case NodeRecurse:
	{
		ReOS_Inst *rec_inst = inst_factory(OpRecurse);
		((StandardInstArgs *)rec_inst->args)->x = node->x;
		pattern->set_inst(pattern, rec_inst, index);
		return index+1;
	}
	}
}

void print_standard_tree(TreeNode *node, PrintTreeNodeFunc print_tree)
{
	switch (node->type) {
	default:
		printf("???");
		break;

	case NodeAlt:
		printf("Alt(");
		print_tree(node->left);
		printf(", ");
		print_tree(node->right);
		printf(")");
		break;

	case NodeCat:
		printf("Cat(");
		print_tree(node->left);
		printf(", ");
		print_tree(node->right);
		printf(")");
		break;

	case NodeDot:
		printf("Dot");
		break;

	case NodeParen:
		printf("Paren(%d, ", node->x);
		print_tree(node->left);
		printf(")");
		break;

	case NodeStar:
		if (node->x)
			printf("Ng");
		printf("Star(");
		print_tree(node->left);
		printf(")");
		break;

	case NodePlus:
		if (node->x)
			printf("Ng");
		printf("Plus(");
		print_tree(node->left);
		printf(")");
		break;

	case NodeQuest:
		if (node->x)
			printf("Ng");
		printf("Quest(");
		print_tree(node->left);
		printf(")");
		break;

	case NodeBacktrack:
		printf("Backtrack(%d)", node->x);
		break;

	case NodeStart:
		printf("Start");
		break;

	case NodeEnd:
		printf("End");
		break;

	case NodeRepCount:
		printf("Rep{%d,%d}(", node->x, node->y);
		print_tree(node->left);
		printf(")");
		break;

	case NodePosAhead:
		printf("PosAhead(");
		print_tree(node->left);
		printf(")");
		break;

	case NodeNegAhead:
		printf("NegAhead(");
		print_tree(node->left);
		printf(")");
		break;
	}
}
