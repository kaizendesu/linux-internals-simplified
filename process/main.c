#include <unistd.h>

int		main(void)
{
	return syscall(1, 1, "Hello standard output!\n", 23);
}
