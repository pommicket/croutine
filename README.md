# croutine

Little proof-of-concept for implementing coroutines in C using `getcontext`/`setcontext`,
with some macro witchcraft to make it pleasant to use:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "croutine.h"

coroutine(const char *, generate_words, size_t n) {
	for (size_t i = 1; i <= n; i++) {
		char *word = memset(calloc(1,1+i), 'm', i);
		yield(word);
		// they'd better not keep that pointer around
		free(word);
	}
}

coroutine(long, generate_fibonacci_infinite) {
	long a = 0, b = 1;
	while (1) {
		long temp = a; a = b; b += temp;
		yield(a);
	}
}

int main(void) {
	const char **words = generator(generate_words, 10);
	while (next(&words)) {
		printf("%s ", *words);
	}
	printf("\n");

	long *fib = generator(generate_fibonacci_infinite);
	for (int i = 0; i < 10; i++) {
		next(&fib);
		printf("%ld ", *fib);
	}
	printf("\n");
	cancel(&fib);
	return 0;
}
```


Requires a Unix-like system with a compiler that supports `__VA_OPT__`.
