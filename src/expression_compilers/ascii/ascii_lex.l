%{
#include "ascii_parse.h"

static char interpret_escape(char c)
{
	switch (c)
	{
		case 'a':
			return '\a';
		case 'b':
			return '\b';
		case 'f':
			return '\f';
		case 'n':
			return '\n';
		case 'r':
			return '\r';
		case 't':
			return '\t';
		case 'v':
			return '\v';
		case '0':
			return '\0';
		default:
			return c;
	}
}

%}

op			[,\|\*\+\?\(\)\.\[\]\-\^\$\!\:\{\}\\=]
specials	[wWsSdDvVhHR]

%%

\\{op}|\\\/ %{
	{
		yylval.c = yytext[1];
		return CHAR;
	}
%}

\{[0-9]+ %{
	{
		yylval.c = atoi(yytext+1);
		return REP_START;
	}
%}

[0-9]+\} %{
	{
		yylval.c = atoi(yytext);
		return REP_END;
	}
%}

\\{specials} %{
	{
		yylval.c = yytext[1];
		return SPECIAL;
	}
%}

\\[a-zA-Z0] %{
	{
		yylval.c = interpret_escape(yytext[1]);
		return CHAR;
	}
%}

\\[1-9]	%{
	{
		yylval.c = yytext[1];
		return ESCAPED_NUM;
	}
%}

\?[0-9] %{
   {
		yylval.c = yytext[1];
		return QUESTION_NUM;
	}
%}

<<EOF>> %{
	{
		return END;
	}
%}

{op} %{
	{
		return yytext[0];
	}
%}

. %{
	{
		yylval.c = yytext[0];
		return CHAR;
	}
%}

%%
int yywrap() { return 1; }
