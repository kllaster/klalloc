#include "utils.h"

t_klalloc_meta *get_klalloc_meta()
{
	static t_klalloc_meta klalloc_meta = {};

	return &klalloc_meta;
}

size_t align(size_t n, size_t round_up_to)
{
	return (n + sizeof(round_up_to) - 1) & ~(sizeof(round_up_to) - 1);
}

inline void *align_addr(void *stack, uintptr_t align)
{
	uintptr_t addr = (uintptr_t)stack;
	addr = (addr + (align - 1)) & -align;
	return (void *)addr;
}