#include "../../include/slab_alloc.h"
#include "test_utils.h"

size_t iter_alloc_slabs = -1;
void *alloc_slabs[100] = {};

void *alloc_slab_test(int order)
{
	alloc_slabs[++iter_alloc_slabs] = aligned_alloc(
		getpagesize(), getpagesize() * (1 << order)
	);
	return alloc_slabs[iter_alloc_slabs];
}

void dealloc_slab_test(void *slab, int order)
{
	(void)order;
	free(slab);
}

void test_allocate_small_objects_and_free()
{
	int *arr[100];
	struct cache cache;

	cache_setup(&cache, alloc_slab_test, dealloc_slab_test, sizeof(int));

	// Allocate
	for (int i = 0; i < 100; i++)
	{
		arr[i] = cache_alloc(&cache);
		assert(arr[i] != NULL);

		char *expected_ptr = ((char *)alloc_slabs[0] + cache.object_size * i);
		assert_print_ptr((char *)arr[i] == expected_ptr, arr[i], expected_ptr);

		*arr[i] = i;
	}

	// Check values and free
	for (int i = 0; i < 100; i++)
	{
		assert(*arr[i] == i);
		cache_free(&cache, arr[i]);
	}

	// Allocate again
	size_t slab_size = cache.object_size * cache.objects_in_slab + sizeof(t_slab);
	int *upper_bound = (int *)alloc_slabs[0] + slab_size;
	for (int i = 0; i < 100; i++)
	{
		arr[i] = cache_alloc(&cache);
		assert(arr[i] != NULL);

		// Pointer must be in bounds of first slab
		assert(arr[i] >= (int *)alloc_slabs[0]);
		assert(arr[i] < upper_bound);
	}

	print_str_literal("[+] Success test slab_alloc allocation small objects and free\n");
}

void slab_tests()
{
	print_str_literal("Slab allocator tests:\n");
	test_allocate_small_objects_and_free();
}
