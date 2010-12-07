#include "ascii_expression.h"
#include "ascii_parse.h"
#include "ascii_lex.h"

TreeNode *ascii_expression_compile(char *string)
{
	YY_BUFFER_STATE buffer = yy_scan_string(string);
	TreeNode *tree = ascii_expression_parse(string);
	yy_delete_buffer(buffer);
	return tree;
}
