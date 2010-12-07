#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ascii_input.h"

ReOS_Input *new_ascii_string_input(char *string)
{
	AsciiStringInputData *data = malloc(sizeof(AsciiStringInputData));
	data->string = string;
	data->pos = 0;
	data->len = strlen(string);

	ReOS_Input *input = malloc(sizeof(ReOS_Input));
	input->indexed_read = input_ascii_string_indexed_read;
	input->stream_read = input_ascii_string_stream_read;
	input->token_size = sizeof(char);
	input->buffer_size = 64;
	input->free_token = 0;
	input->data = data;
	return input;
}

void free_ascii_string_input(ReOS_Input *input)
{
	if (input) {
		free(input->data);
		free(input);
	}
}

int input_ascii_string_indexed_read(void *buf, int size, long index, void *d)
{
	AsciiStringInputData *data = (AsciiStringInputData *)d;
	int copied = index+size < data->len ? size : data->len-index;
	memcpy(buf, data->string+index, copied);
	return copied;
}

int input_ascii_string_stream_read(void *buf, int size, void *d)
{
	AsciiStringInputData *data = (AsciiStringInputData *)d;
	int copied = data->pos+size < data->len ? size : data->len-data->pos;
	memcpy(buf, data->string+data->pos, copied);
	data->pos += copied;
	return copied;
}

ReOS_Input *new_ascii_file_input(char *path)
{
	ReOS_Input *input = malloc(sizeof(ReOS_Input));
	input->indexed_read = input_ascii_file_indexed_read;
	input->stream_read = input_ascii_file_stream_read;
	input->token_size = sizeof(char);
	input->buffer_size = 1024;
	input->free_token = 0;
	input->data = fopen(path, "r");
	if (!input->data) {
		fprintf(stderr, "error: could not open file ");
		perror(path);
		exit(1);
	}
	return input;
}

void free_ascii_file_input(ReOS_Input *input)
{
	if (input) {
		fclose(input->data);
		free(input);
	}
}

int input_ascii_file_stream_read(void *buf, int size, void *file)
{
	return fread(buf, 1, size, file);
}

int input_ascii_file_indexed_read(void *buf, int size, long index, void *file)
{
	fseek(file, index, SEEK_SET);
	return fread(buf, 1, size, file);
}

void print_ascii_string_input(ReOS_Kernel *k)
{
	printf("%s\n", ((AsciiStringInputData *)k->token_buf->input->data)->string);

	int i;
	for (i = 0; i < k->sp; i++)
		printf(" ");
	printf("^\n");
}
