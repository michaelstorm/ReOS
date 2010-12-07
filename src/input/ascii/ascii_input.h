#ifndef ASCII_INPUT_H
#define ASCII_INPUT_H

#include "reos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AsciiStringInputData AsciiStringInputData;

struct AsciiStringInputData
{
	char *string;
	long pos;
	long len;
};

ReOS_Input *new_ascii_string_input(char *);
void free_ascii_string_input(ReOS_Input *);
int input_ascii_string_stream_read(void *, int, void *);
int input_ascii_string_indexed_read(void *, int, long, void *);

ReOS_Input *new_ascii_file_input(char *);
void free_ascii_file_input(ReOS_Input *);
int input_ascii_file_stream_read(void *, int, void *);
int input_ascii_file_indexed_read(void *, int, long, void *);

void print_ascii_string_input(ReOS_Kernel *);

#ifdef __cplusplus
}
#endif

#endif
