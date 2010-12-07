#ifndef UNICODE_INPUT_H
#define UNICODE_INPUT_H

#include "reos_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UnicodeStringInputData UnicodeStringInputData;
typedef struct UnicodeFileInputData UnicodeFileInputData;
struct UFILE;
struct UText;

struct UnicodeStringInputData
{
	char *utf8;
	struct UText *utext;
	int status;
};

struct UnicodeFileInputData
{
	char *utf8;
	struct UFILE *file;
	int status;
};

ReOS_Input *new_unicode_string_input(char *);
void free_unicode_string_input(ReOS_Input *);
int input_unicode_string_stream_read(void *, int, void *);
int input_unicode_string_indexed_read(void *, int, long, void *);

ReOS_Input *new_unicode_file_input(char *, char *);
void free_unicode_file_input(ReOS_Input *);
int input_unicode_file_stream_read(void *, int, void *);
int input_unicode_file_indexed_read(void *, int, long, void *);

void print_unicode_string_input(ReOS_Kernel *);

#ifdef __cplusplus
}
#endif

#endif
