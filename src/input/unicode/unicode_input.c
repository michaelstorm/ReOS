#include <stdio.h>
#include <stdlib.h>
#include "unicode_input.h"
#include "unicode/ubrk.h"
#include "unicode/ustdio.h"

ReOS_Input *new_unicode_string_input(char *utf8)
{
	UnicodeStringInputData *data = malloc(sizeof(UnicodeStringInputData));
	data->utf8 = utf8;
	data->status = U_ZERO_ERROR;
	data->utext = utext_openUTF8(0, utf8, -1, &data->status);

	ReOS_Input *input = malloc(sizeof(ReOS_Input));
	input->indexed_read = input_unicode_string_indexed_read;
	input->stream_read = input_unicode_string_stream_read;
	input->token_size = sizeof(UChar32);
	input->buffer_size = 1024;
	input->free_token = 0;
	input->data = data;
	return input;
}

void free_unicode_string_input(ReOS_Input *input)
{
	if (input) {
		utext_close(((UnicodeStringInputData *)input->data)->utext);
		free(input->data);
		free(input);
	}
}

int input_unicode_string_indexed_read(void *buf, int size, long index, void *d)
{
	UnicodeStringInputData *data = (UnicodeStringInputData *)d;
	UTEXT_SETNATIVEINDEX(data->utext, 0);

	long i;
	for (i = 0; i < index; i++) {
		if (UTEXT_NEXT32(data->utext) == U_SENTINEL)
			break;
	}

	for (i = 0; i < size; i++) {
		UChar32 next = UTEXT_NEXT32(data->utext);
		if (next == U_SENTINEL)
			break;
		((UChar32 *)buf)[i] = next;
	}
	return i;
}

int input_unicode_string_stream_read(void *buf, int size, void *d)
{
	UnicodeStringInputData *data = (UnicodeStringInputData *)d;

	int i;
	UChar32 next;
	for (i = 0; i < size; i++) {
		next = UTEXT_NEXT32(data->utext);
		if (next == U_SENTINEL)
			break;
		((UChar32 *)buf)[i] = next;
	}
	return i;
}

ReOS_Input *new_unicode_file_input(char *path, char *codepage)
{
	ReOS_Input *input = malloc(sizeof(ReOS_Input));
	input->indexed_read = input_unicode_file_indexed_read;
	input->stream_read = input_unicode_file_stream_read;
	input->token_size = sizeof(UChar32);
	input->buffer_size = 1024;
	input->free_token = 0;

	UnicodeFileInputData *data = malloc(sizeof(UnicodeFileInputData));
	data->status = U_ZERO_ERROR;
	data->file = u_fopen(path, "r", NULL, codepage);
	if (!data->file) {
		fprintf(stderr, "error: could not open file %s", path);
		exit(1);
	}

	input->data = data;
	return input;
}

void free_unicode_file_input(ReOS_Input *input)
{
	if (input) {
		u_fclose(((UnicodeFileInputData *)input->data)->file);
		free(input->data);
		free(input);
	}
}

int input_unicode_file_stream_read(void *buf, int size, void *d)
{
	UnicodeFileInputData *data = (UnicodeFileInputData *)d;

	int i;
	for (i = 0; i < size; i++) {
		UChar32 next = u_fgetcx(data->file);
		if (next == U_SENTINEL || next == U_EOF)
			break;
		((UChar32 *)buf)[i] = next;
	}
	return i;
}

int input_unicode_file_indexed_read(void *buf, int size, long index, void *d)
{
	UnicodeFileInputData *data = (UnicodeFileInputData *)d;
	u_frewind(data->file);

	long i;
	for (i = 0; i < index; i++) {
		if (u_fgetcx(data->file) == U_EOF)
			break;
	}

	UChar32 next;
	for (i = 0; i < size; i++) {
		next = u_fgetcx(data->file);
		if (next == U_EOF)
			break;
		((UChar32 *)buf)[i] = next;
	}
	return i;
}

void print_unicode_string_input(ReOS_Kernel *k)
{
	printf("%s\n", ((UnicodeStringInputData *)k->token_buf->input->data)->utf8);

	int i;
	for (i = 0; i < k->sp; i++)
		printf(" ");
	printf("^\n");
}
