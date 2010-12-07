#ifndef REOS_BUFFER_H
#define REOS_BUFFER_H

#include "reos_types.h"

#define token_at(token_buf, index) \
	(&((char *)(token_buf)->buf)[(index) * (token_buf)->input->token_size])

#ifndef reos_tokenbuffer_consume
#define reos_tokenbuffer_consume(token_buf) \
	token_at(token_buf, (token_buf)->pos++)
#endif

/*
	if the entire buffer has been consumed,
		read more input
		if no more input is available,
			return 0
	otherwise
		consume and return next token
*/
#ifndef reos_tokenbuffer_read
#define reos_tokenbuffer_read(token_buf) \
	(((token_buf)->pos == (token_buf)->len) ? \
		reos_tokenbuffer_input(token_buf), \
		(!(token_buf)->len ? 0 \
			: \
			reos_tokenbuffer_consume(token_buf)) \
		: \
		reos_tokenbuffer_consume(token_buf))
#endif

#ifdef __cplusplus
extern "C" {
#endif

ReOS_TokenBuffer *new_reos_tokenbuffer(ReOS_Input *);
void free_reos_tokenbuffer(ReOS_TokenBuffer *);
void reos_tokenbuffer_input(ReOS_TokenBuffer *);
void reos_tokenbuffer_fastforward(ReOS_TokenBuffer *, int);

#ifdef __cplusplus
}
#endif

#endif
