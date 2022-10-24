#include "btags_alloc.h"
#include "test_utils.h"

static const int buffer_size = 4096 * 4;
static char buffer[buffer_size];

static void inline allocate_small_objects(t_btags_alloc_meta *btags_alloc_meta)
{
	int size = 200;
	char **arr = btags_alloc(btags_alloc_meta, sizeof(char *) * size);
	assert(arr != NULL);

	for (int i = 0; i < size; i++)
	{
		arr[i] = btags_alloc(btags_alloc_meta, 31);
		assert(arr[i] != NULL);
	}

	for (int i = 0; i < size; i++)
	{
		btags_alloc_free(btags_alloc_meta, arr[i]);
	}
	btags_alloc_free(btags_alloc_meta, arr);
}

void test_btags_alloc_allocation()
{
	t_btags_alloc_meta *btags_alloc_meta = (t_btags_alloc_meta *)buffer;

	btags_alloc_setup(
		btags_alloc_meta,
		(char *)buffer + sizeof(t_btags_alloc_meta),
		buffer_size - sizeof(t_btags_alloc_meta)
	);

	allocate_small_objects(btags_alloc_meta);
	allocate_small_objects(btags_alloc_meta);
	allocate_small_objects(btags_alloc_meta);

	print_str_literal("[+] Success test btags_alloc allocation\n");
}

void test_btags_alloc_fail_allocation()
{
	t_btags_alloc_meta *btags_alloc_meta = (t_btags_alloc_meta *)buffer;

	btags_alloc_setup(
		btags_alloc_meta,
		(char *)buffer + sizeof(t_btags_alloc_meta),
		buffer_size - sizeof(t_btags_alloc_meta)
	);

	char **arr = btags_alloc(btags_alloc_meta, sizeof(char *) * 10);
	assert(arr != NULL);

	for (int i = 0; i < 10; i++)
	{
		arr[i] = btags_alloc(btags_alloc_meta, 1500);
		assert(arr[i] != NULL);
	}

	assert(btags_alloc(btags_alloc_meta, 1000) == NULL);

	for (int i = 0; i < 10; i++)
	{
		btags_alloc_free(btags_alloc_meta, arr[i]);
	}
	btags_alloc_free(btags_alloc_meta, arr);

	print_str_literal("[+] Success test btags_alloc fail allocation\n");
}

void test_btags_alloc_valid_data()
{
	t_btags_alloc_meta *btags_alloc_meta = (t_btags_alloc_meta *)buffer;

	btags_alloc_setup(
		btags_alloc_meta,
		(char *)buffer + sizeof(t_btags_alloc_meta),
		buffer_size - sizeof(t_btags_alloc_meta)
	);

	char **arr = btags_alloc(btags_alloc_meta, sizeof(char *) * 10);
	assert(arr != NULL);

	for (int i = 0; i < 10; i++)
	{
		arr[i] = btags_alloc(btags_alloc_meta, 4);
		assert(arr[i] != NULL);
		arr[i][0] = '1';
		arr[i][1] = '2';
		arr[i][2] = '3';
		arr[i][3] = '\0';
	}

	for (int i = 0; i < 4; i++)
	{
		btags_alloc_free(btags_alloc_meta, arr[i]);
	}

	for (int i = 4; i < 10; i++)
	{
		assert(arr[i][0] == '1');
		assert(arr[i][1] == '2');
		assert(arr[i][2] == '3');
		assert(arr[i][3] == '\0');
	}

	for (int i = 4; i < 10; i++)
	{
		btags_alloc_free(btags_alloc_meta, arr[i]);
	}
	btags_alloc_free(btags_alloc_meta, arr);

	print_str_literal("[+] Success test btags_alloc valid data\n");
}

void test_btags_alloc_defragmentation()
{
	t_btags_alloc_meta *btags_alloc_meta = (t_btags_alloc_meta *)buffer;

	btags_alloc_setup(
		btags_alloc_meta,
		(char *)buffer + sizeof(t_btags_alloc_meta),
		buffer_size - sizeof(t_btags_alloc_meta)
	);

	{
		char *arr = btags_alloc(btags_alloc_meta, 1400);
		assert(arr != NULL);
		btags_alloc_free(btags_alloc_meta, arr);
	}

	{
		char **arr = btags_alloc(btags_alloc_meta, sizeof(char *) * 20);
		assert(arr != NULL);

		for (int i = 0; i < 20; i++)
		{
			arr[i] = btags_alloc(btags_alloc_meta, 4);
			assert(arr[i] != NULL);
		}

		for (int i = 0; i < 20; i++)
		{
			btags_alloc_free(btags_alloc_meta, arr[i]);
		}
		btags_alloc_free(btags_alloc_meta, arr);
	}

	{
		char *arr = btags_alloc(btags_alloc_meta, 1400);
		assert(arr != NULL);
		btags_alloc_free(btags_alloc_meta, arr);
	}

	print_str_literal("[+] Success test btags_alloc defragmentation\n");
}

void test_btags_alloc_check_max_node_size()
{
	t_btags_alloc_meta *btags_alloc_meta = (t_btags_alloc_meta *)buffer;

	btags_alloc_setup(
		btags_alloc_meta,
		(char *)buffer + sizeof(t_btags_alloc_meta),
		buffer_size - sizeof(t_btags_alloc_meta)
	);

	int arr_size = 10;
	char **arr = btags_alloc(btags_alloc_meta, sizeof(char *) * arr_size);

	size_t size_without_arr =
		buffer_size - sizeof(t_btags_alloc_meta) - (sizeof(t_btags_alloc_node) * 2)
		- (sizeof(char *) * arr_size + sizeof(t_btags_alloc_node) * 2);
	assert_print_size_t(
		btags_alloc_meta->max_node_size == size_without_arr,
		btags_alloc_meta->max_node_size,
		size_without_arr
	);

	size_t expected_size;
	for (int i = 0; i < arr_size; i++)
	{
		arr[i] = btags_alloc(btags_alloc_meta, 16);
		assert(arr[i] != NULL);

		expected_size = size_without_arr - (i + 1) * (16 + sizeof(t_btags_alloc_node) * 2);
		assert_print_size_t(
			btags_alloc_meta->max_node_size == expected_size,
			btags_alloc_meta->max_node_size,
			expected_size
		);
	}

	for (int i = arr_size - 1; i >= 0; i--)
	{
		btags_alloc_free(btags_alloc_meta, arr[i]);

		expected_size = size_without_arr - i * (16 + sizeof(t_btags_alloc_node) * 2);
		assert_print_size_t(
			btags_alloc_meta->max_node_size == expected_size,
			btags_alloc_meta->max_node_size,
			expected_size
		);
	}

	btags_alloc_free(btags_alloc_meta, arr);

	size_t full_memory = buffer_size - sizeof(t_btags_alloc_meta) - (sizeof(t_btags_alloc_node) * 2);
	assert_print_size_t(
		btags_alloc_meta->max_node_size == full_memory,
		btags_alloc_meta->max_node_size,
		full_memory
	);

	print_str_literal("[+] Success test btags_alloc check max_node_size\n");
}

void test_btags_alloc_check_ptr_in_btags_alloc_list()
{
	t_btags_alloc_meta *btags_alloc_meta = (t_btags_alloc_meta *)buffer;

	btags_alloc_setup(
		btags_alloc_meta,
		(char *)buffer + sizeof(t_btags_alloc_meta),
		buffer_size - sizeof(t_btags_alloc_meta)
	);

	char **arr = btags_alloc(btags_alloc_meta, sizeof(char *) * 10);
	assert(arr != NULL);

	for (int i = 0; i < 10; i++)
	{
		arr[i] = btags_alloc(btags_alloc_meta, 100);
		assert(arr[i] != NULL);
		assert(ptr_in_btags_alloc_list(btags_alloc_meta, arr[i]));
	}

	assert(btags_alloc(btags_alloc_meta, 20000) == NULL);

	for (int i = 0; i < 10; i++)
	{
		btags_alloc_free(btags_alloc_meta, arr[i]);
	}
	btags_alloc_free(btags_alloc_meta, arr);

	assert(!ptr_in_btags_alloc_list(btags_alloc_meta, (void *)0x01));

	print_str_literal("[+] Success test btags_alloc check ptr in btags_alloc list\n");
}

void btags_alloc_tests()
{
	print_str_literal("Btags allocator tests:\n");
	test_btags_alloc_allocation();
	test_btags_alloc_fail_allocation();
	test_btags_alloc_valid_data();
	test_btags_alloc_defragmentation();
	test_btags_alloc_check_max_node_size();
	test_btags_alloc_check_ptr_in_btags_alloc_list();
}
