#include <antlr3.h>
#include "string_expression.h"
#include "string_expressionLexer.h"
#include "string_expressionParser.h"
#include "string_tree.h"

TreeNode *string_expression_compile(char *string)
{
	pANTLR3_INPUT_STREAM input = antlr3NewAsciiStringCopyStream((pANTLR3_UINT8)string, strlen(string), 0);
	pstring_expressionLexer lex = string_expressionLexerNew(input);
	pANTLR3_COMMON_TOKEN_STREAM tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lex));
	pstring_expressionParser parser = string_expressionParserNew(tokens);

	TreeNode *tree = parser->line(parser);

	parser->free(parser);
	tokens->free(tokens);
	lex->free(lex);
	input->close(input);
	return tree;
}
