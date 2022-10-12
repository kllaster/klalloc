#include "../../include/slab_alloc.h"
#include "../../include/utils.h"
#include "printf.h"
#include "test_utils.h"

static const int buffer_size = 12288;

static char buffer[buffer_size];

void *get_slab_mem()
{
	return align_addr(&buffer[buffer_size / 2], 4096);
}

void *alloc_slab_test(size_t order)
{
	(void)order;
	return get_slab_mem();
}

void dealloc_slab_test(void *slab, size_t order)
{
	(void)slab;
	(void)order;
}

void test_slab_alloc_allocation_objects_and_free()
{
	char *arr[100];
	struct cache cache;

	cache_setup(&cache, alloc_slab_test, dealloc_slab_test, 8);

	// Allocate
	for (int i = 0; i < 100; i++)
	{
		arr[i] = cache_alloc(&cache);
		assert(arr[i] != NULL);

		char *expected_ptr = ((char *)get_slab_mem() + cache.object_size * i);
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
	char *upper_bound = (char *)get_slab_mem() + slab_size;
	for (int i = 0; i < 100; i++)
	{
		arr[i] = cache_alloc(&cache);
		assert(arr[i] != NULL);

		// Pointer must be in bounds of first slab
		assert(arr[i] >= (char *)get_slab_mem());
		assert(arr[i] < upper_bound);
	}

	cache_release(&cache);

	print_str_literal("[+] Success test slab_alloc allocation small objects and free\n");
}

void test_slab_alloc_defragmentation()
{
	char *arr[100];
	struct cache cache;

	cache_setup(&cache, alloc_slab_test, dealloc_slab_test, 8);

	// Allocate
	for (int i = 0; i < 100; i++)
	{
		arr[i] = cache_alloc(&cache);
		assert(arr[i] != NULL);

		char *expected_ptr = ((char *)get_slab_mem() + cache.object_size * i);
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

	cache_release(&cache);

	print_str_literal("[+] Success test slab_alloc defragmentation\n");
}

void test_slab_alloc_check_ptr_in_cache()
{
	int *arr[100];
	struct cache cache;

	cache_setup(&cache, alloc_slab_test, dealloc_slab_test, 8);

	// Allocate
	for (int i = 0; i < 100; i++)
	{
		arr[i] = cache_alloc(&cache);
		assert(arr[i] != NULL);

		char *expected_ptr = ((char *)get_slab_mem() + cache.object_size * i);
		assert_print_ptr((char *)arr[i] == expected_ptr, arr[i], expected_ptr);
	}

	for (int i = 0; i < 100; i++)
	{
		assert(ptr_in_cache(&cache, arr[i]));
		cache_free(&cache, arr[i]);
	}

	assert(!ptr_in_cache(&cache, (void *)0x01));

	print_str_literal("[+] Success test slab_alloc check ptr in cache\n");
}

void slab_tests()
{
	print_str_literal("Slab allocator tests:\n");
	test_slab_alloc_allocation_objects_and_free();
	test_slab_alloc_defragmentation();
	test_slab_alloc_check_ptr_in_cache();
}
