#include "klalloc.h"

void *alloc_slab_mmamp(int order)
{
	return mmap(
		NULL, getpagesize() * (1 << order), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
	);
}

void free_slab_mmap(void *slab, int order)
{
	munmap(slab, getpagesize() * (1 << order));
}
