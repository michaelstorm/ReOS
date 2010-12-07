// Copyright 2007-2009 Russ Cox.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

%{

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ascii_tree.h"

extern int yylex();
static void yyerror(char*);
TreeNode *parsed_ascii_tree;
static int nparen;

struct TreeNode;

/**/

%}

/* pisses me off that the preprocessor inserts this before including
   ascii_tree.h for whatever reason */
%union {
	struct TreeNode *node;
	int c;
	int nparen;
}

%token	<c>				ESCAPED_NUM QUESTION_NUM CHAR REP_START REP_END END SPECIAL
%type	<c>				char
%type	<node>	 		lit alt concat repeat single line range repeatrange
%type	<nparen>		count

%%

line: alt END
	{
		parsed_ascii_tree = $1;
		return 1;
	}

alt:
	concat
|	alt '|' concat
	{
		$$ = new_ascii_tree_node(NodeAlt, $1, $3);
	}
;

concat:
	repeat
|	concat repeat
	{
		$$ = new_ascii_tree_node(NodeCat, $1, $2);
	}
;

repeat:
	single
|	single '*'
	{
		$$ = new_ascii_tree_node(NodeStar, $1, 0);
	}
|	single '*' '?'
	{
		$$ = new_ascii_tree_node(NodeStar, $1, 0);
		$$->x = 1;
	}
|	single '+'
	{
		$$ = new_ascii_tree_node(NodePlus, $1, 0);
	}
|	single '+' '?'
	{
		$$ = new_ascii_tree_node(NodePlus, $1, 0);
		$$->x = 1;
	}
|	single '?'
	{
		$$ = new_ascii_tree_node(NodeQuest, $1, 0);
	}
|	single '?' '?'
	{
		$$ = new_ascii_tree_node(NodeQuest, $1, 0);
		$$->x = 1;
	}
|	single REP_START '}'
	{
		$$ = new_ascii_tree_node(NodeRepCount, $1, 0);
		$$->x = $2;
		$$->y = 0;
	}
|	single REP_START ',' '}'
	{
		$$ = new_ascii_tree_node(NodeRepCount, $1, 0);
		$$->x = $2;
		$$->y = -1;
	}
|	single '{' ',' REP_END
	{
		$$ = new_ascii_tree_node(NodeRepCount, $1, 0);
		$$->x = 0;
		$$->y = $4;
	}
|	single REP_START ',' REP_END
	{
		$$ = new_ascii_tree_node(NodeRepCount, $1, 0);
		$$->x = $2;
		$$->y = $4;
	}
|	'(' '?' '=' alt ')'
	{
		$$ = new_ascii_tree_node(NodePosAhead, $4, 0);
	}
|	'(' '?' '!' alt ')'
	{
		$$ = new_ascii_tree_node(NodeNegAhead, $4, 0);
	}
|	'(' QUESTION_NUM ')'
	{
		$$ = new_ascii_tree_node(NodeRecurse, 0, 0);
		$$->x = $2;
	}
;

count:
	{
		$$ = nparen++;
	}
;

single:
	'(' count alt ')'
	{
		$$ = new_ascii_tree_node(NodeParen, $3, 0);
		$$->x = $2;
	}
|	'(' '?' ':' alt ')'
	{
		$$ = $4;
	}
|	'[' '^' range ']'
	{
		TreeNode *negahead = new_ascii_tree_node(NodeNegAhead, $3, 0);
		TreeNode *dot = new_ascii_tree_node(NodeDot, 0, 0);

		$$ = new_ascii_tree_node(NodeCat, negahead, dot);
	}
|	'[' range ']'
	{
		$$ = $2;
	}
|	'^'
	{
		$$ = new_ascii_tree_node(NodeStart, 0, 0);
	}
|	'$'
	{
		$$ = new_ascii_tree_node(NodeEnd, 0, 0);
	}
|	ESCAPED_NUM
	{
		$$ = new_ascii_tree_node(NodeBacktrack, 0, 0);
		$$->x = $1-48-1;
	}
|	'.'
	{
		$$ = new_ascii_tree_node(NodeDot, 0, 0);
	}
|	lit
;

range:
	repeatrange
|	range repeatrange
	{
		$$ = new_ascii_tree_node(NodeAlt, $1, $2);
	}
;

repeatrange:
	char '-' char
	{
		$$ = new_ascii_tree_node(NodeAsciiRange, 0, 0);
		AsciiTreeNodeArgs *args = $$->args;
		args->c1 = $1;
		args->c2 = $3;
	}
|	lit
|	'?'
	{
		$$ = new_ascii_tree_node(NodeAsciiChar, 0, 0);
		((AsciiTreeNodeArgs *)$$->args)->c1 = '?';
	}
;

char:
	CHAR
;

lit:
	char
	{
		$$ = new_ascii_tree_node(NodeAsciiChar, 0, 0);
		((AsciiTreeNodeArgs *)$$->args)->c1 = $1;
	}
|	SPECIAL
	{
		$$ = new_ascii_tree_node(NodeAsciiChar, 0, 0);
		((AsciiTreeNodeArgs *)$$->args)->c1 = -1;
		((AsciiTreeNodeArgs *)$$->args)->c2 = $1;
	}
|	':'
	{
		$$ = new_ascii_tree_node(NodeAsciiChar, 0, 0);
		((AsciiTreeNodeArgs *)$$->args)->c1 = ':';
	}
;

%%

static void yyerror(char *s)
{
	fprintf(stderr, "Yacc error: %s\n", s);
}

TreeNode* ascii_expression_parse(char *s)
{
	parsed_ascii_tree = 0;
	nparen = 0;

	if (yyparse() != 1)
		yyerror("did not parse");
	return parsed_ascii_tree;
}
