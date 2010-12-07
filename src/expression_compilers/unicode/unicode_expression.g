grammar unicode_expression;

options {
	language=C;
	backtrack=true;
}

@header {
	#include "unicode_tree.h"
	#include "unicode/ustring.h"
}

@postinclude {
	static int nparen = 1;
}

line returns [TreeNode *node]:
	alt EOF
	{
		return new_unicode_tree_node(NodeParen, $alt.node, 0);
	}
;

alt returns [TreeNode *node]:
	left=concat {$node = $left.node;}
	('|' right=concat {$node = new_unicode_tree_node(NodeAlt, $node, $right.node);})*
;

concat returns [TreeNode *node]:
	left=repeat {$node = $left.node;}
	(right=repeat {$node = new_unicode_tree_node(NodeCat, $node, $right.node);})*
;

repeat returns [TreeNode *node]:
	single '*' greedy='?'?
	{
		$node = new_unicode_tree_node(NodeStar, $single.node, 0);
		if ($greedy)
			$node->x = 1;
	}
|	single '+' greedy='?'?
	{
		$node = new_unicode_tree_node(NodePlus, $single.node, 0);
		if ($greedy)
			$node->x = 1;
	}
|	single '?' greedy='?'?
	{
		$node = new_unicode_tree_node(NodeQuest, $single.node, 0);
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
		$node = new_unicode_tree_node(NodePosAhead, $alt.node, 0);
	}
|	'(' '?' '!' alt ')'
	{
		$node = new_unicode_tree_node(NodeNegAhead, $alt.node, 0);
	}
|	single
	{
		$node = $single.node;
	}
;

single returns [TreeNode *node]:
	'(' alt ')'
	{
		$node = new_unicode_tree_node(NodeParen, $alt.node, 0);
		$node->x = nparen++;
	}
|	'(' '?' ':' alt ')'
	{
		$node = $alt.node;
	}
|	'^'
	{
		$node = new_unicode_tree_node(NodeStart, 0, 0);
	}
|	range
	{
		$node = $range.node;
	}
|	'$'
	{
		$node = new_unicode_tree_node(NodeEnd, 0, 0);
	}
|	ESCAPED_INT
	{
		$node = new_unicode_tree_node(NodeBacktrack, 0, 0);
		$node->x = $ESCAPED_INT.text->toInt32($ESCAPED_INT.text);
	}
|	'.'
	{
		$node = new_unicode_tree_node(NodeDot, 0, 0);
	}
|	lit
	{
		$node = $lit.node;
	}
;

range returns [TreeNode *node]:
	'[' negated='^'?
	left=range_component {$node = $left.node;}
	(right=range_component {$node = new_unicode_tree_node(NodeAlt, $node, $right.node);})*
	']'
	{
		if ($negated) {

			TreeNode *negahead = new_unicode_tree_node(NodeNegAhead, $node, 0);
			TreeNode *dot = new_unicode_tree_node(NodeDot, 0, 0);

			$node = new_unicode_tree_node(NodeCat, negahead, dot);
		}
	}
;

range_component returns [TreeNode *node]:
	char1=character '-' char2=character
	{
		$node = new_unicode_tree_node(NodeUnicodeRange, 0, 0);
		$node->x = $char1.c;
		$node->y = $char2.c;
	}
|	lit
	{
		$node = $lit.node;
	}
;

character returns [UChar32 c]:
	CHAR
	{
		int32_t utf8_len;
		int32_t utf16_len;
		UChar utf16[4];
		UErrorCode status = 0;

		u_strFromUTF8(utf16, 4, &utf16_len, &$CHAR.text->chars[0], $CHAR.text->len, &status);
		u_strToUTF32(&$c, 1, 0, utf16, utf16_len, &status);
	}
|	ESCAPED_CHAR
	{
		int32_t utf8_len;
		int32_t utf16_len;
		UChar utf16[4];
		UErrorCode status = 0;

		u_strFromUTF8(utf16, 4, &utf16_len, &$ESCAPED_CHAR.text->chars[1], $ESCAPED_CHAR.text->len-1, &status);
		u_strToUTF32(&$c, 1, 0, utf16, utf16_len, &status);
	}
|	ESCAPED_OPERATOR
	{
		int32_t utf8_len;
		int32_t utf16_len;
		UChar utf16[4];
		UErrorCode status = 0;

		u_strFromUTF8(utf16, 4, &utf16_len, &$ESCAPED_OPERATOR.text->chars[1], $ESCAPED_OPERATOR.text->len-1, &status);
		u_strToUTF32(&$c, 1, 0, utf16, utf16_len, &status);
	}
;

lit returns [TreeNode *node]:
	character
	{
		$node = new_unicode_tree_node(NodeUnicodeChar, 0, 0);
		((UnicodeTreeNodeArgs *)$node->args)->c1 = $character.c;
	}
|	SPECIAL
	{
		$node = new_unicode_tree_node(NodeUnicodeChar, 0, 0);
		UnicodeTreeNodeArgs *args = (UnicodeTreeNodeArgs *)$node->args;
		args->c1 = -1;

		int32_t utf8_len;
		int32_t utf16_len;
		UChar utf16[4];
		UErrorCode status = 0;

		u_strFromUTF8(utf16, 4, &utf16_len, &$SPECIAL.text->chars[0], $SPECIAL.text->len, &status);
		u_strToUTF32(&args->c2, 1, 0, utf16, utf16_len, &status);
	}
;

ESCAPED_OPERATOR :	'\\' OPERATOR;

// fucking lexer needs this ('%')? kludge
OPERATOR :	('%')? ('*' | '?' | '+' | '[' | ']' | '(' | ')' | '{' | '}' | '^' | '$' | '.' | '-' | '|' | ':' | '!' | '=' | ',');

ESCAPED_INT	:	'\\' '1'..'9';

SPECIAL	:	'\\' ('w' | 'W' | 's' | 'S' | 'd' | 'D'
				 | 'v' | 'V' | 'h' | 'H' | 'R'
				 )
		;

COMMENT
    :   '//' ~('\n'|'\r')* '\r'? '\n' {$channel=HIDDEN;}
    |   '/*' ( options {greedy=false;} : . )* '*/' {$channel=HIDDEN;}
	;

ESCAPED_CHAR:	'\\' CHAR;

REP_START :	'{' '0'..'9'+;

REP_END :	'0'..'9'+ '}';

CHAR:
	('\u0000'..'\u00bf')
|	('\u00c2'..'\u00df' UTF8_TRAIL)
|	('\u00e0'..'\u00ef' UTF8_TRAIL UTF8_TRAIL)
|	('\u00f0'..'\u00f4' UTF8_TRAIL UTF8_TRAIL UTF8_TRAIL)
;

fragment
UTF8_TRAIL: ('\u0080'..'\u00bf');

fragment
HEX_DIGIT : ('0'..'9'|'a'..'f'|'A'..'F') ;

fragment
ESC_SEQ
    :   '\\' ('b'|'t'|'n'|'f'|'r'|'\"'|'\''|'\\')
    |   UNICODE_ESC
    |   OCTAL_ESC
    ;

fragment
OCTAL_ESC
    :   '\\' ('0'..'3') ('0'..'7') ('0'..'7')
    |   '\\' ('0'..'7') ('0'..'7')
    |   '\\' ('0'..'7')
    ;

fragment
UNICODE_ESC
    :   '\\' 'u' HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT
    ;
