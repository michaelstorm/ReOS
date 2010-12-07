#include <antlr3.h>
#include "unicode_expression.h"
#include "unicode_expressionLexer.h"
#include "unicode_expressionParser.h"
#include "unicode_tree.h"

TreeNode *unicode_expression_compile(char *string)
{
	pANTLR3_INPUT_STREAM input = antlr3NewAsciiStringCopyStream((pANTLR3_UINT8)string, strlen(string), 0);
	punicode_expressionLexer lex = unicode_expressionLexerNew(input);
	pANTLR3_COMMON_TOKEN_STREAM tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lex));
	punicode_expressionParser parser = unicode_expressionParserNew(tokens);

	TreeNode *tree = parser->line(parser);

	parser->free(parser);
	tokens->free(tokens);
	lex->free(lex);
	input->close(input);
	return tree;
}
