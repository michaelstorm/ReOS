grammar shell_debug;

options {
	language=C;
}

@header {
	#include <stdlib.h>
	#include "shell_debugger.h"
	#include "reos_list.h"
	#include "reos_types.h"
}

line returns [ShellDebuggerCommand *command]:
	WORD
	{
		$command = malloc(sizeof(ShellDebuggerCommand));
		$command->name = malloc(sizeof(char)*32);
		strncpy($command->name, $WORD.text->chars, 31);
		$command->options = new_reos_simplelist((VoidPtrFunc)free_shell_debugger_option);
	}
	(option
		{
			reos_simplelist_push_tail($command->options, $option.op);
		}
	)*
;

option returns [ShellDebuggerOption *op]:
	name=WORD '=' value=INT
	{
		$op = malloc(sizeof(ShellDebuggerOption));
		$op->key_name = malloc(sizeof(char)*32);
		strncpy($op->key_name, $name.text->chars, 31);
		$op->value_is_num = 1;
		$op->value_num = $value.text->toInt32($value.text);
	}
|	name=WORD '=' value=WORD
	{
		$op = malloc(sizeof(ShellDebuggerOption));
		$op->key_name = malloc(sizeof(char)*32);
		strncpy($op->key_name, $name.text->chars, 31);
		$op->value_is_num = 0;
		$op->value_str = malloc(sizeof(char)*32);
		strncpy($op->value_str, $value.text->chars, 31);
	}
|	value=WORD
	{
		$op = malloc(sizeof(ShellDebuggerOption));
		$op->key_name = 0;
		$op->value_is_num = 0;
		$op->value_str = malloc(sizeof(char)*32);
		strncpy($op->value_str, $value.text->chars, 31);
	}
|	value=INT
	{
		$op = malloc(sizeof(ShellDebuggerOption));
		$op->key_name = 0;
		$op->value_is_num = 1;
		$op->value_num = $value.text->toInt32($value.text);
	}
;

OPERATORS :	'='
	;

INT :	'0'..'9'+
	;

WORD :	('a'..'z')+
	 ;

WS : (' ' | '\t' | '\n' | '\r')+
	 {$channel=HIDDEN;}
   ;
