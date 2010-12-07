#ifndef ASCII_EXPRESSION_COMPILE_H
#define ASCII_EXPRESSION_COMPILE_H

#include "standard_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

TreeNode *ascii_expression_compile(char *);
TreeNode *ascii_expression_parse(char *); // in the grammar file

#ifdef __cplusplus
}
#endif

#endif
