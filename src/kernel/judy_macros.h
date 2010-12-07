#define judy_iter_begin_body(type, data, list, iter, pvalue) \
	type *data; \
	type **pvalue; \
	Word_t iter = 0; \
	JLF(pvalue, list, iter); \
	if (pvalue != NULL) \
		data = *pvalue; \
	while (pvalue != NULL)

#define judy_iter_next_body(data, list, iter, pvalue) \
	{ \
		JLN(pvalue, list, iter); \
		if (pvalue != NULL) \
			data = *pvalue; \
	}

#define judy_iter_end_body(type, data, list, iter, pvalue) \
	type *data; \
	type **pvalue; \
	Word_t iter = -1; \
	JLL(pvalue, list, iter); \
	if (pvalue != NULL) \
		data = *pvalue; \
	while (pvalue != NULL)

#define judy_iter_prev_body(data, list, iter, pvalue) \
	{ \
		JLP(pvalue, list, iter); \
		if (pvalue != NULL) \
			data = *pvalue; \
	}

#define judy_cxx_iter_begin_body(type, data, list, iter, pvalue) \
	type *data; \
	void *pvalue; \
	Word_t iter = 0; \
	JLF(pvalue, list, iter); \
	if (pvalue != NULL) \
		data = *(type **)pvalue; \
	while (pvalue != NULL)

#define judy_cxx_iter_next_body(type, data, list, iter, pvalue) \
	{ \
		JLN(pvalue, list, iter); \
		if (pvalue != NULL) \
			data = *(type **)pvalue; \
	}

#define judy_iter_begin(type, data, list) judy_iter_begin_body(type, data, (list), iter_##data, pvalue_##data)
#define reos_judylist_iter_begin(type, data, list) judy_iter_begin(type, data, ((list)->impl->judy))

#define judy_iter_next(data, list) judy_iter_next_body(data, (list), iter_##data, pvalue_##data)
#define reos_judylist_iter_next(data, list) judy_iter_next(data, ((list)->impl->judy))

#define judy_iter_end(type, data, list) judy_iter_end_body(type, data, (list), iter_##data, pvalue_##data)
#define reos_judylist_iter_end(type, data, list) judy_iter_end(type, data, ((list)->impl->judy))

#define judy_iter_prev(data, list) judy_iter_prev_body(data, (list), iter_##data, pvalue_##data)
#define reos_judylist_iter_prev(data, list) judy_iter_prev(data, ((list)->impl->judy))

#define judy_cxx_iter_begin(type, data, list) judy_cxx_iter_begin_body(type, data, (list), iter_##data, pvalue_##data)
#define reos_judylist_cxx_iter_begin(type, data, list) judy_cxx_iter_begin(type, data, ((list)->impl->judy))

#define judy_cxx_iter_next(type, data, list) judy_cxx_iter_next_body(type, data, (list), iter_##data, pvalue_##data)
#define reos_judylist_cxx_iter_next(data, list) judy_cxx_iter_next(data, ((list)->impl->judy))

#define reos_judylist_detach(l) \
	if ((l)->impl->refs > 1) { \
		(l)->impl->refs--; \
		(l)->impl = reos_judylistimpl_clone((l)->impl); \
	}

#define judy_insert(list, index, data) \
	{ \
		void **pvalue; \
		JLI(pvalue, (list), (index)); \
		*pvalue = (data); \
	}

#define reos_judylist_insert(list, index, data) \
	reos_judylist_detach(list) \
	judy_insert((list)->impl->judy, index, data);

#define judy_get(list, index, data) \
	{ \
		void **pvalue; \
		JLG(pvalue, (list), (index)); \
		if (pvalue) \
			(data) = *pvalue; \
		else \
			(data) = 0; \
	}

#define judy_try_get_body(list, index, type, data, pvalue) \
	type *data; \
	void **pvalue; \
	JLG(pvalue, (list), (index)); \
	if (pvalue && ((data) = *pvalue, 1))

#define judy_try_get(list, index, type, data) \
	judy_try_get_body(list, index, type, data, pvalue_##data)

#define judy_fail_get_body(list, index, type, data, pvalue) \
	type *data; \
	void **pvalue; \
	JLG(pvalue, (list), (index)); \
	if (!pvalue || ((data) = *pvalue, 0))

#define judy_fail_get(list, index, type, data) \
	judy_fail_get_body(list, index, type, data, pvalue_##data)

#define reos_judylist_fail_get(list, index, type, data) \
	judy_fail_get((list)->impl->judy, index, type, data)

#define reos_judylist_get(list, index, data) \
	judy_get((list)->impl->judy, index, data);

#define judy_delete(list, index) \
	{ \
		int rc; \
		JLD(rc, (list), (index)); \
	}

#define reos_judylist_delete(list, index) \
	reos_judylist_detach(list) \
	judy_delete((list)->impl->judy, index);

#define judy_first(list, data) \
	{ \
		void **pvalue; \
		Word_t iter = 0; \
		JLF(pvalue, (list), iter); \
		if (pvalue != NULL) \
			(data) = *pvalue; \
		else \
			(data) = 0; \
	}

#define reos_judylist_first(list, data) \
	judy_first((list)->impl->judy, data)

#define judy_last(list, data) \
	{ \
		void **pvalue; \
		Word_t iter = -1; \
		JLL(pvalue, (list), iter); \
		if (pvalue != NULL) \
			(data) = *pvalue; \
		else \
			(data) = 0; \
	}

#define reos_judylist_last(list, data) \
	judy_last((list)->impl->judy, data)

#define judy_last_index(list, index) \
	{ \
		void **pvalue; \
		(index) = -1; \
		JLL(pvalue, (list), (index)); \
	}

#define reos_judylist_last_index(list, index) \
	judy_last_index((list)->impl->judy, index);

#define judy_first_absent_index(list, index) \
	{ \
		int rc; \
		(index) = 0; \
		JLFE(rc, (list), (index)); \
	}

#define reos_judylist_first_absent_index(list, index) \
	judy_first_absent_index((list)->impl->judy, index);

#define if_judy_is_empty(list) \
	void *judy_is_empty_pvalue; \
	Word_t i = 0; \
	JLF(judy_is_empty_pvalue, (list), i); \
	if (!judy_is_empty_pvalue)

#define if_judylist_is_empty(list) \
	if_judy_is_empty((list)->impl->judy)

#define if_judy_is_not_empty(list) \
	void *judy_is_not_empty_pvalue; \
	Word_t i = 0; \
	JLF(judy_is_not_empty_pvalue, (list), i); \
	if (judy_is_not_empty_pvalue)

#define if_judylist_is_not_empty(list) \
	if_judy_is_not_empty((list)->impl->judy)

#define if_judy_has(list, index) \
	void **pvalue_##index; \
	JLG(pvalue_##index, (list), (index)); \
	if (pvalue_##index != NULL)

#define if_judylist_has(list, index) \
	if_judy_has((list)->impl->judy, index)
