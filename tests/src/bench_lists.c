//#include <Judy.h>
#include "pikevm_types.h"
//#include "utarray.h"
#include "pikevm_stdlib.h"
#include "pikevm_list.h"

int main(int argc, char **argv)
{
	int j;
	int times = 20;//atoi(argv[1]);
	int compound_size = 8;//atoi(argv[2]);

	/*Pvoid_t judy = NULL;
	for (j = 0; j < times; j++)
		judy_insert(judy, j, (void *)0x1);

	judy_iter_begin(void, x, judy)
		judy_iter_next(x, judy);

	free_judy(judy, 0);*/

	CompoundList *compound = new_compoundlist(compound_size, 0, 0);
	for (j = 0; j < times; j++)
		compoundlist_push_tail(compound, (void *)0x1);

	foreach_compound(void, y, compound)
	{}

	for (j = 0; j < times; j++)
		compoundlist_pop_head(compound);

	free_compoundlist(compound);

	/*SimpleList *simple = new_simplelist(0);
	for (j = 0; j < times; j++)
		simplelist_push_head(simple, (void *)0x1);

	foreach_simple(void, z, simple)
	{}

	for (j = 0; j < times; j++)
		simplelist_pop_head(simple);

	free_simplelist(simple);*/

	/*UT_array *ut;
	utarray_new(ut, &ut_int_icd);
	utarray_reserve(ut, compound_size);

	for (j = 0; j < times; j++)
		utarray_push_back(ut, &j);

	int *p;
	for (p = (int *)utarray_front(ut); p != NULL; p = (int *)utarray_next(ut, p))
	{}

	while ((p = (int *)utarray_back(ut)))
		utarray_pop_back(ut);

	utarray_free(ut);

	hi_handle_t *hi_handle;
	const char *data_ptr;*/

	/* initialize hashish handle */
//	hi_init_str(&hi_handle, 23);

	/* insert an key/data pair */
//	ret = hi_insert_str(hi_handle, key, data);

	/* search for a pair with a string key and store result */
//	hi_get_str(hi_handle, key, &data_ptr);

//	fprintf(stdout, "Key: %s Data: %s\n", key, data_ptr);

	/* free the hashish handle */
//	hi_fini(hi_handle);*/

	return 0;
}
