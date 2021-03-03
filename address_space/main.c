#include <stdio.h>
#include <stdlib.h>

static int static_var;
int global_var;

int		main(void)
{
	int		stack_var = 0;
	int		*heap_var;

	heap_var = malloc(sizeof(int));
	printf("static_var: %p\nglobal_var: %p\n", &static_var, &global_var);
	printf("stack_var: %p\nheap_var: %p\n", &stack_var, heap_var);
	free(heap_var);

	/*
	 * We infinitely loop so that we can use the /proc filesystem
	 * to analyze this process after running it.
	 */
	for(;;);

	return 0;
}
