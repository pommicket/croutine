#include "croutine.h"

#include <ucontext.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <stdalign.h>
#include <stddef.h>

typedef struct {
	alignas(max_align_t)
	ucontext_t context;
	ucontext_t rtrn;
	bool reentrant;
	bool finished;
} Generator;

static Generator *generator_from_data(void *data) {
	return (Generator *)((char *)data - sizeof(Generator));
}

void yield_(void *data) {
	Generator *gen = generator_from_data(data);
	gen->reentrant = false;
	getcontext(&gen->context);
	if (gen->reentrant) {
		return;
	}
	gen->reentrant = true;
	setcontext(&gen->rtrn);
}

static void next_internal(Generator *gen) {
	gen->reentrant = false;
	getcontext(&gen->rtrn);
	if (!gen->finished && !gen->reentrant) {
		gen->reentrant = true;
		setcontext(&gen->context);
		// should never happen
		abort();
	}
}

static void gen_destroy(Generator *gen) {
	munmap(gen->context.uc_stack.ss_sp,
		gen->context.uc_stack.ss_size);
	free(gen);
}

bool next_(void *pdata_v) {
	void **pdata = pdata_v;
	Generator *gen = generator_from_data(*pdata);
	if (!gen) {
		fprintf(stderr, "next() on an empty generator\n");
		abort();
	}
	next_internal(gen);
	if (gen->finished) {
		gen_destroy(gen);
		*pdata = NULL;
		return false;
	}
	return true;
}

static void context_function(void *data, void (*f)(void *, size_t), size_t arg) {
	f(data, arg);
	generator_from_data(data)->finished = true;
}

void *generator_(void (*f)(void *, size_t), size_t item_size, size_t arg) {
	size_t bytes_to_allocate = item_size + sizeof(Generator);
	// round up to multiple of sizeof(max_align_t)
	bytes_to_allocate = (bytes_to_allocate + sizeof(max_align_t) - 1)
		/ sizeof(max_align_t) * sizeof(max_align_t);
	Generator *gen = malloc(bytes_to_allocate);
	void *data = gen + 1;
	if (!gen) {
		perror("malloc"); abort();
	}
	size_t max_stack_size = sizeof(size_t) < 8
		? 8UL << 20
		: 256UL << 20;
	void *stack = mmap(NULL, max_stack_size, PROT_READ|PROT_WRITE,
		MAP_ANONYMOUS|MAP_SHARED, -1, 0);
	if (stack == MAP_FAILED) {
		perror("mmap"); abort();
	}
	memset(data, 0, item_size);
	gen->finished = false;
	gen->reentrant = false;
	getcontext(&gen->context);
	gen->reentrant = true;
	gen->context.uc_link = &gen->rtrn;
	gen->context.uc_stack = (stack_t){
		.ss_size = max_stack_size,
		.ss_sp = stack,
	};
	makecontext(&gen->context, (void(*)(void))context_function, 3, data, f, arg);
	return data;
}

void cancel_(void *pdata_v) {
	void **pdata = pdata_v;
	gen_destroy(generator_from_data(*pdata));
	*pdata = NULL;
}
