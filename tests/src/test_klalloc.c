#include "klalloc.h"
#include "test_utils.h"

static char **allocate_small_objs(size_t size)
{
	char **arr;

	arr = klalloc(sizeof(char *) * size);
	assert(arr != NULL);

	for (size_t i = 0; i < size; i++)
	{
		arr[i] = klalloc(6);
		assert(arr[i] != NULL);
		arr[i][0] = '1';
		arr[i][1] = '2';
		arr[i][2] = '3';
		arr[i][3] = '4';
		arr[i][4] = '5';
		arr[i][5] = '\0';
	}

	for (size_t i = 0; i < size; i++)
	{
		assert(arr[i] != NULL);
		assert_print_int(arr[i][0] == '1', arr[i][0], 1);
		assert_print_int(arr[i][1] == '2', arr[i][1], 2);
		assert_print_int(arr[i][2] == '3', arr[i][2], 3);
		assert_print_int(arr[i][3] == '4', arr[i][3], 4);
		assert_print_int(arr[i][4] == '5', arr[i][4], 5);
		assert_print_int(arr[i][5] == '\0', arr[i][5], 0);
	}
	return arr;
}

void test_klalloc_small_allocation_and_free()
{
	char **arr;
	size_t size = 1250;

	arr = allocate_small_objs(size);

	for (size_t i = 0; i < size; i++)
		klfree(arr[i]);
	klfree(arr);

	print_str_literal("[+] Success test klalloc small allocation and free\n");
}

static char **allocate_big_objs(size_t size)
{
	char **arr;
	size_t obj_size = getpagesize() * 4;

	arr = klalloc(sizeof(char *) * size);
	assert(arr != NULL);

	for (size_t i = 0; i < size; i++)
	{
		arr[i] = klalloc(obj_size);
		for (size_t a = 0; a < obj_size; a++)
			arr[i][a] = a;
		assert(arr[i] != NULL);
	}
	return arr;
}

void test_klalloc_big_allocation_and_free()
{
	char **arr;
	size_t size = 21;

	arr = allocate_big_objs(size);

	for (size_t i = 0; i < size; i++)
		klfree(arr[i]);
	klfree(arr);

	print_str_literal("[+] Success test klalloc big allocation and free\n");
}

void test_klalloc_small_medium_big_allocation_and_free()
{
	size_t arr_small_size = 1201;
	size_t arr_small_two_size = 3001;
	size_t arr_big_size = 21;
	char **arr_small, **arr_small_two, **arr_big;

	arr_small = allocate_small_objs(arr_small_size);
	arr_big = allocate_big_objs(arr_big_size);

	for (size_t i = 0; i < arr_big_size; i++)
		klfree(arr_big[i]);

	arr_small_two = allocate_small_objs(arr_small_two_size);

	for (size_t i = 0; i < arr_small_size; i++)
		klfree(arr_small[i]);

	for (size_t i = 0; i < arr_small_two_size; i++)
		klfree(arr_small_two[i]);

	klfree(arr_small_two);
	klfree(arr_small);
	klfree(arr_big);

	print_str_literal("[+] Success test klalloc small, medium, big allocation and free\n");
}

void klalloc_tests()
{
	print_str_literal("klalloc tests:\n");
	test_klalloc_small_allocation_and_free();
	test_klalloc_big_allocation_and_free();
	test_klalloc_small_medium_big_allocation_and_free();
}
