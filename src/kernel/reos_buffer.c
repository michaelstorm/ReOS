#include "reos_buffer.h"
#include "reos_stdlib.h"

ReOS_TokenBuffer *new_reos_tokenbuffer(ReOS_Input *input)
{
	ReOS_TokenBuffer *token_buf = malloc(sizeof(ReOS_TokenBuffer));
	token_buf->len = 0;
	token_buf->pos = 0;
	token_buf->buf = malloc(input->token_size * input->buffer_size);
	token_buf->input = input;
	return token_buf;
}

void free_reos_tokenbuffer(ReOS_TokenBuffer *token_buf)
{
	if (token_buf) {
		free(token_buf->buf);
		free(token_buf);
	}
}

void reos_tokenbuffer_input(ReOS_TokenBuffer *token_buf)
{
	if (token_buf->input->free_token) {
		int i;
		for (i = 0; i < token_buf->len; i++)
			token_buf->input->free_token(token_at(token_buf, i));
	}

	token_buf->len = token_buf->input->stream_read(token_buf->buf, token_buf->input->buffer_size,
												   token_buf->input->data);
	token_buf->pos = 0;
}

void reos_tokenbuffer_fastforward(ReOS_TokenBuffer *token_buf, int offset)
{
	ReOS_Input *input = token_buf->input;
	if (input->stream_read) {
		void **buffer = token_buf->buf;

		int read = 0;
		int remaining = offset;
		while (read <= offset) {
			remaining -= token_buf->len;
			token_buf->len = input->stream_read(buffer, input->buffer_size, input->data);
			read += token_buf->len;
		}

		token_buf->pos = remaining;
	}
}
