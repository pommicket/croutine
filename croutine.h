#ifndef CROUTINE_H_
#define CROUTINE_H_

#include <stdbool.h>
#include <stddef.h>

// internals
void *generator_(void (*)(void *, size_t), size_t, size_t);
#define check_function_type_(function, ...) \
	(void)sizeof function((void *)0 __VA_OPT__(, (size_t)0))
#define get_rid_of_(...)
#define deparenthesize_(...) __VA_ARGS__
void yield_(void *);
bool next_(void *);
void cancel_(void *);

#define coroutine(type, name, ...) typedef type generator_type_for_##name##_; void name(type *_gen __VA_OPT__(, __VA_ARGS__))
#define yield(value) (*(_gen) = (value), yield_(_gen))
// Just check that gen is actually a pointer-to-pointer
#define next(gen) ((void)sizeof **(gen), next_(gen))
// Just check that gen is actually a pointer-to-pointer
#define cancel(gen) ((void)sizeof **(gen), cancel_(gen))
#define generator(function, ...) \
	(check_function_type_(function __VA_OPT__(, __VA_ARGS__)), \
	(generator_type_for_##function##_ *)generator_((void(*)(void*,size_t))function, \
		sizeof (generator_type_for_##function##_) \
		/* ugly macro nonsense to add , 0 if __VA_ARGS__ is empty */ \
		deparenthesize_ __VA_OPT__(()) __VA_OPT__(get_rid_of_)(, 0) \
		__VA_OPT__(, __VA_ARGS__)))

#endif // CROUTINE_H_
