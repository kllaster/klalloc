#include "slab_alloc.h"
#include "test_utils.h"

static const int buffer_size = 4096 * 6;

static char buffer[buffer_size];

static int next_slab = -1;

void *get_slab_mem(int next, size_t size)
{
	return (char *)align((size_t) & buffer, size) + size * next;
}

void *alloc_slab_test(size_t size)
{
	return get_slab_mem(++next_slab, size);
}

void dealloc_slab_test(void *slab, size_t order)
{
	(void)slab;
	(void)order;
	next_slab--;
}

void test_slab_alloc_allocation_objects_and_free()
{
	char *arr[100];
	t_slab_cache cache;

	cache_setup(&cache, alloc_slab_test, dealloc_slab_test, 8);

	// Allocate
	for (int i = 0; i < 100; i++)
	{
		arr[i] = cache_alloc(&cache);
		assert(arr[i] != NULL);

		char *expected_ptr = ((char *)get_slab_mem(0, cache.slab_size) + cache.object_size * i);
		assert_print_ptr((char *)arr[i] == expected_ptr, arr[i], expected_ptr);

		arr[i][0] = '1';
		arr[i][1] = '2';
		arr[i][2] = '3';
		arr[i][3] = '4';
		arr[i][4] = '5';
		arr[i][5] = '6';
		arr[i][6] = '7';
		arr[i][7] = '\0';
	}

	// Check values and free
	for (int i = 0; i < 100; i++)
	{
		assert(arr[i][0] == '1');
		assert(arr[i][1] == '2');
		assert(arr[i][2] == '3');
		assert(arr[i][3] == '4');
		assert(arr[i][4] == '5');
		assert(arr[i][5] == '6');
		assert(arr[i][6] == '7');
		assert(arr[i][7] == '\0');
		cache_free(&cache, arr[i]);
	}

	// Allocate again
	size_t slab_size = cache.object_size * cache.objects_in_slab + sizeof(t_slab);
	char *upper_bound = (char *)get_slab_mem(1, cache.slab_size) + slab_size;
	for (int i = 0; i < 100; i++)
	{
		arr[i] = cache_alloc(&cache);
		assert(arr[i] != NULL);

		// Pointer must be in bounds of first slab
		assert(arr[i] >= (char *)get_slab_mem(0, cache.slab_size));
		assert(arr[i] < upper_bound);
		cache_free(&cache, arr[i]);
	}

	cache_release(&cache);

	print_str_literal("[+] Success test slab_alloc allocation small objects and free\n");
}

void test_slab_alloc_defragmentation()
{
	int size = 500;
	char *arr[size];
	t_slab_cache cache;

	cache_setup(&cache, alloc_slab_test, dealloc_slab_test, 8);

	// Allocate
	for (int i = 0; i < size; i++)
	{
		arr[i] = cache_alloc(&cache);
		assert(arr[i] != NULL);

		char *expected_ptr;
		expected_ptr = ((char *)get_slab_mem(0, cache.slab_size) + cache.object_size * i);
		assert_print_ptr((char *)arr[i] == expected_ptr, arr[i], expected_ptr);
	}

	// Save old ptr and free
	char *saved_ptr[10];
	for (int i = 0; i < 10; i++)
	{
		saved_ptr[i] = arr[i];
		cache_free(&cache, arr[i]);
		assert_print_ptr(
			cache.partially_slabs->first_free_obj == saved_ptr[i],
			cache.partially_slabs->first_free_obj,
			saved_ptr[i]
		)
	}
	for (int i = 0, a = 9; i < 10; i++, a--)
	{
		arr[i] = (char *)cache_alloc(&cache);
		assert(arr[i] != NULL);
		assert_print_ptr(arr[i] == saved_ptr[a], arr[i], saved_ptr[a]);
	}

	for (int i = 0; i < size; i++)
	{
		cache_free(&cache, arr[i]);
	}

	cache_release(&cache);

	print_str_literal("[+] Success test slab_alloc defragmentation\n");
}

void test_slab_alloc_check_ptr_in_cache()
{
	int size = 200;
	int *arr[size];
	t_slab_cache cache;

	cache_setup(&cache, alloc_slab_test, dealloc_slab_test, 8);

	// Allocate
	for (int i = 0; i < size; i++)
	{
		arr[i] = cache_alloc(&cache);
		assert(arr[i] != NULL);

		char *expected_ptr;
		if (i < 257)
			expected_ptr = ((char *)get_slab_mem(0, cache.slab_size) + cache.object_size * i);
		else
			expected_ptr = ((char *)get_slab_mem(1, cache.slab_size)
			                + cache.object_size * (i - 257));
		assert_print_ptr((char *)arr[i] == expected_ptr, arr[i], expected_ptr);
	}

	for (int i = 0; i < size; i++)
	{
		assert(ptr_in_cache(&cache, arr[i]));
		cache_free(&cache, arr[i]);
	}

	assert(!ptr_in_cache(&cache, (void *)0x01));

	cache_release(&cache);

	print_str_literal("[+] Success test slab_alloc check ptr in cache\n");
}

void slab_tests()
{
	print_str_literal("Slab allocator tests:\n");
	test_slab_alloc_allocation_objects_and_free();
	test_slab_alloc_defragmentation();
	test_slab_alloc_check_ptr_in_cache();
}
