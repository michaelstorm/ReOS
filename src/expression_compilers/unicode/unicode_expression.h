// Copyright 2007-2009 Russ Cox.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef UNICODE_EXPRESSION_H
#define UNICODE_EXPRESSION_H

#include "standard_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

TreeNode *unicode_expression_compile(char *);

#ifdef __cplusplus
}
#endif

#endif
