grammar string_expression;

options {
	language=C;
	backtrack=true;
}

@header {
	#include "string_tree.h"
}

@postinclude {
	static int nparen = 1;
}

line returns [TreeNode *node]:
	alt EOF
	{
		return new_string_tree_node(NodeParen, $alt.node, 0);
	}
;

alt returns [TreeNode *node]:
	left=concat {$node = $left.node;}
	('|' right=concat {$node = new_string_tree_node(NodeAlt, $node, $right.node);})*
;

concat returns [TreeNode *node]:
	left=repeat {$node = $left.node;}
	(right=repeat {$node = new_string_tree_node(NodeCat, $node, $right.node);})*
;

repeat returns [TreeNode *node]:
	single '*' greedy='?'?
	{
		$node = new_string_tree_node(NodeStar, $single.node, 0);
		if ($greedy)
			$node->x = 1;
	}
|	single '+' greedy='?'?
	{
		$node = new_string_tree_node(NodePlus, $single.node, 0);
		if ($greedy)
			$node->x = 1;
	}
|	single '?' greedy='?'?
	{
		$node = new_string_tree_node(NodeQuest, $single.node, 0);
		if ($greedy)
			$node->x = 1;
	}
|	single REP_START '}'
	{
		$node = new_unicode_tree_node(NodeRepCount, $single.node, 0);
		$node->x = atoi($REP_START.text->chars+1);
		$node->y = 0;
	}
|	single REP_START ',' '}'
	{
		$node = new_unicode_tree_node(NodeRepCount, $single.node, 0);
		$node->x = atoi($REP_START.text->chars+1);
		$node->y = -1;
	}
|	single '{' ',' REP_END
	{
		$node = new_unicode_tree_node(NodeRepCount, $single.node, 0);
		$node->x = 0;
		$node->y = atoi($REP_END.text->chars);
	}
|	single REP_START ',' REP_END
	{
		$node = new_unicode_tree_node(NodeRepCount, $single.node, 0);
		$node->x = atoi($REP_START.text->chars+1);
		$node->y = atoi($REP_END.text->chars);
	}
|	'(' '?' '=' alt ')'
	{
		$node = new_string_tree_node(NodePosAhead, $alt.node, 0);
	}
|	'(' '?' '!' alt ')'
	{
		$node = new_string_tree_node(NodeNegAhead, $alt.node, 0);
	}
|	single
	{
		$node = $single.node;
	}
;

single returns [TreeNode *node]:
	'(' alt ')'
	{
		$node = new_string_tree_node(NodeParen, $alt.node, 0);
		$node->x = nparen++;
	}
|	'(' '?' ':' alt ')'
	{
		$node = $alt.node;
	}
|	'^'
	{
		$node = new_string_tree_node(NodeStart, 0, 0);
	}
|	range
	{
		$node = $range.node;
	}
|	'$'
	{
		$node = new_string_tree_node(NodeEnd, 0, 0);
	}
|	ESCAPED_INT
	{
		$node = new_string_tree_node(NodeBacktrack, 0, 0);
		$node->x = $ESCAPED_INT.text->chars[1]-48;
	}
|	'.'
	{
		$node = new_string_tree_node(NodeDot, 0, 0);
	}
|	recognizer
	{
		$node = $recognizer.node;
	}
;

range returns [TreeNode *node]:
	'[' negated='^'?
	left=recognizer {$node = $left.node;}
	(right=recognizer {$node = new_string_tree_node(NodeAlt, $node, $right.node);})*
	']'
	{
		if ($negated) {
			TreeNode *negahead = new_string_tree_node(NodeNegAhead, $node, 0);
			TreeNode *dot = new_string_tree_node(NodeDot, 0, 0);

			$node = new_string_tree_node(NodeCat, negahead, dot);
		}
	}
;

recognizer returns [TreeNode *node]:
	WORD
	{
		$node = new_string_tree_node(NodeString, 0, 0);
		strcpy($node->args, $WORD.text->chars);
	}
;

// fucking lexer needs this ('%')? kludge
OPERATOR : ('%')? ('*' | '?' | '+' | '(' | ')' | '[' | ']' | '{' | '}' | '^' | '$' | '|' | ':' | '!' | '=' | ',');

ESCAPED_INT	:	'\\' '1'..'9';

REP_START :	'{' '0'..'9'+;

REP_END :	'0'..'9'+ '}';

COMMENT
	:   '//' ~('\n'|'\r')* '\r'? '\n' {$channel=HIDDEN;}
	|   '/*' ( options {greedy=false;} : . )* '*/' {$channel=HIDDEN;}
	;

WS : (' ' | '\t' | '\n' | '\r')+
	 {$channel=HIDDEN;}
   ;

WORD : ('a'..'z' | 'A'..'Z' | '0'..'9' | '-' | '_' | '.')+;
