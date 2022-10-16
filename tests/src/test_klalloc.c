#include "../../include/klalloc.h"
#include "test_utils.h"

static const int SMALL_OBJ = 6;
static const int MEDIUM_OBJ = 4096;
static const int BIG_OBJ = 4096 * 4;

static inline void write_num_in_mem(char *mem, size_t size)
{
	for (size_t i = 0; i < size; i++)
		mem[i] = i;
}

static inline void check_num_in_mem(char *mem, size_t size)
{
	for (size_t i = 0; i < size; i++)
		assert_print_int(mem[i] == (char)i, mem[i], (int)i);
}

static char **allocate_arr_objs(size_t obj_size, size_t size)
{
	char **arr;

	arr = klalloc(sizeof(char *) * size);
	assert(arr != NULL);

	for (size_t i = 0; i < size; i++)
	{
		arr[i] = klalloc(obj_size);
		assert(arr[i] != NULL);
		write_num_in_mem(arr[i], obj_size);
	}

	for (size_t i = 0; i < size; i++)
		check_num_in_mem(arr[i], obj_size);

	return arr;
}

void test_klalloc_small_allocation_and_free()
{
	char **arr;
	size_t size = 1250;

	arr = allocate_arr_objs(SMALL_OBJ, size);

	for (size_t i = 0; i < size; i++)
		klfree(arr[i]);
	klfree(arr);

	print_str_literal("[+] Success test klalloc small allocation and free\n");
}

void test_klalloc_medium_allocation_and_free()
{
	char **arr;
	size_t size = 21;

	arr = allocate_arr_objs(MEDIUM_OBJ, size);

	for (size_t i = 0; i < size; i++)
		klfree(arr[i]);
	klfree(arr);

	show_alloc_mem();
	print_str_literal("[+] Success test klalloc medium allocation and free\n");
}

void test_klalloc_big_allocation_and_free()
{
	char **arr;
	size_t size = 15;

	arr = allocate_arr_objs(BIG_OBJ, size);

	for (size_t i = 0; i < size; i++)
		klfree(arr[i]);
	klfree(arr);

	print_str_literal("[+] Success test klalloc big allocation and free\n");
}

void test_klalloc_small_medium_big_allocation_and_free()
{
	size_t arr_small_size = 1201;
	size_t arr_medium_size = 21;
	size_t arr_big_size = 15;
	char **arr_small, **arr_medium, **arr_big;

	arr_small = allocate_arr_objs(SMALL_OBJ, arr_small_size);
	arr_medium = allocate_arr_objs(MEDIUM_OBJ, arr_medium_size);
	arr_big = allocate_arr_objs(BIG_OBJ, arr_big_size);

	for (size_t i = 0; i < arr_small_size; i++)
		klfree(arr_small[i]);

	for (size_t i = 0; i < arr_medium_size; i++)
		klfree(arr_medium[i]);

	for (size_t i = 0; i < arr_big_size; i++)
		klfree(arr_big[i]);

	klfree(arr_small);
	klfree(arr_medium);
	klfree(arr_big);

	print_str_literal("[+] Success test klalloc small, medium, big allocation and free\n");
}

void test_klalloc_reallocation()
{
	char *mem;

	mem = klalloc(8);
	assert(mem != NULL);
	write_num_in_mem(mem, 8);
	check_num_in_mem(mem, 8);

	mem = klrealloc(mem, 10);
	assert(mem != NULL);
	check_num_in_mem(mem, 8);
	write_num_in_mem(mem, 10);
	check_num_in_mem(mem, 10);

	mem = klrealloc(mem, 31);
	assert(mem != NULL);
	check_num_in_mem(mem, 10);
	write_num_in_mem(mem, 31);
	check_num_in_mem(mem, 31);

	mem = klrealloc(mem, 1221);
	assert(mem != NULL);
	check_num_in_mem(mem, 31);
	write_num_in_mem(mem, 1221);
	check_num_in_mem(mem, 1221);

	mem = klrealloc(mem, 8000);
	assert(mem != NULL);
	check_num_in_mem(mem, 1221);
	write_num_in_mem(mem, 8000);
	check_num_in_mem(mem, 8000);

	klfree(mem);

	print_str_literal("[+] Success test klrealloc\n");
}

void klalloc_tests()
{
	print_str_literal("klalloc tests:\n");
	test_klalloc_small_allocation_and_free();
	test_klalloc_medium_allocation_and_free();
	test_klalloc_big_allocation_and_free();
	test_klalloc_small_medium_big_allocation_and_free();
	test_klalloc_reallocation();
}
