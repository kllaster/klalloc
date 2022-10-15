#include "body_alloc.h"
#include "test_utils.h"

static const int buffer_size = 4096 * 4;
static char buffer[buffer_size];

static void inline allocate_small_objects(t_body_alloc_meta *body_alloc_meta)
{
	int size = 200;
	char **arr = body_alloc(body_alloc_meta, sizeof(char *) * size);
	assert(arr != NULL);

	for (int i = 0; i < size; i++)
	{
		arr[i] = body_alloc(body_alloc_meta, 31);
		assert(arr[i] != NULL);
	}

	for (int i = 0; i < size; i++)
	{
		body_alloc_free(body_alloc_meta, arr[i]);
	}
	body_alloc_free(body_alloc_meta, arr);
}

void test_body_alloc_allocation()
{
	t_body_alloc_meta *body_alloc_meta = (t_body_alloc_meta *)buffer;

	body_alloc_setup(
		body_alloc_meta,
		(char *)buffer + sizeof(t_body_alloc_meta),
		buffer_size - sizeof(t_body_alloc_meta)
	);

	allocate_small_objects(body_alloc_meta);
	allocate_small_objects(body_alloc_meta);
	allocate_small_objects(body_alloc_meta);

	print_str_literal("[+] Success test body_alloc allocation\n");
}

void test_body_alloc_fail_allocation()
{
	t_body_alloc_meta *body_alloc_meta = (t_body_alloc_meta *)buffer;

	body_alloc_setup(
		body_alloc_meta,
		(char *)buffer + sizeof(t_body_alloc_meta),
		buffer_size - sizeof(t_body_alloc_meta)
	);

	char **arr = body_alloc(body_alloc_meta, sizeof(char *) * 10);
	assert(arr != NULL);

	for (int i = 0; i < 10; i++)
	{
		arr[i] = body_alloc(body_alloc_meta, 1500);
		assert(arr[i] != NULL);
	}

	assert(body_alloc(body_alloc_meta, 1000) == NULL);

	for (int i = 0; i < 10; i++)
	{
		body_alloc_free(body_alloc_meta, arr[i]);
	}
	body_alloc_free(body_alloc_meta, arr);

	print_str_literal("[+] Success test body_alloc fail allocation\n");
}

void test_body_alloc_valid_data()
{
	t_body_alloc_meta *body_alloc_meta = (t_body_alloc_meta *)buffer;

	body_alloc_setup(
		body_alloc_meta,
		(char *)buffer + sizeof(t_body_alloc_meta),
		buffer_size - sizeof(t_body_alloc_meta)
	);

	char **arr = body_alloc(body_alloc_meta, sizeof(char *) * 10);
	assert(arr != NULL);

	for (int i = 0; i < 10; i++)
	{
		arr[i] = body_alloc(body_alloc_meta, 4);
		assert(arr[i] != NULL);
		arr[i][0] = '1';
		arr[i][1] = '2';
		arr[i][2] = '3';
		arr[i][3] = '\0';
	}

	for (int i = 0; i < 4; i++)
	{
		body_alloc_free(body_alloc_meta, arr[i]);
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
		body_alloc_free(body_alloc_meta, arr[i]);
	}
	body_alloc_free(body_alloc_meta, arr);

	print_str_literal("[+] Success test body_alloc valid data\n");
}

void test_body_alloc_defragmentation()
{
	t_body_alloc_meta *body_alloc_meta = (t_body_alloc_meta *)buffer;

	body_alloc_setup(
		body_alloc_meta,
		(char *)buffer + sizeof(t_body_alloc_meta),
		buffer_size - sizeof(t_body_alloc_meta)
	);

	{
		char *arr = body_alloc(body_alloc_meta, 1400);
		assert(arr != NULL);
		body_alloc_free(body_alloc_meta, arr);
	}

	{
		char **arr = body_alloc(body_alloc_meta, sizeof(char *) * 20);
		assert(arr != NULL);

		for (int i = 0; i < 20; i++)
		{
			arr[i] = body_alloc(body_alloc_meta, 4);
			assert(arr[i] != NULL);
		}

		for (int i = 0; i < 20; i++)
		{
			body_alloc_free(body_alloc_meta, arr[i]);
		}
		body_alloc_free(body_alloc_meta, arr);
	}

	{
		char *arr = body_alloc(body_alloc_meta, 1400);
		assert(arr != NULL);
		body_alloc_free(body_alloc_meta, arr);
	}

	print_str_literal("[+] Success test body_alloc defragmentation\n");
}

void test_body_alloc_check_max_node_size()
{
	t_body_alloc_meta *body_alloc_meta = (t_body_alloc_meta *)buffer;

	body_alloc_setup(
		body_alloc_meta,
		(char *)buffer + sizeof(t_body_alloc_meta),
		buffer_size - sizeof(t_body_alloc_meta)
	);

	int arr_size = 10;
	char **arr = body_alloc(body_alloc_meta, sizeof(char *) * arr_size);

	size_t size_without_arr =
		buffer_size - sizeof(t_body_alloc_meta) - (sizeof(t_body_alloc_node) * 2)
		- (sizeof(char *) * arr_size + sizeof(t_body_alloc_node) * 2);
	assert_print_size_t(
		body_alloc_meta->max_node_size == size_without_arr,
		body_alloc_meta->max_node_size,
		size_without_arr
	);

	size_t expected_size;
	for (int i = 0; i < arr_size; i++)
	{
		arr[i] = body_alloc(body_alloc_meta, 16);
		assert(arr[i] != NULL);

		expected_size = size_without_arr - (i + 1) * (16 + sizeof(t_body_alloc_node) * 2);
		assert_print_size_t(
			body_alloc_meta->max_node_size == expected_size,
			body_alloc_meta->max_node_size,
			expected_size
		);
	}

	for (int i = arr_size - 1; i >= 0; i--)
	{
		body_alloc_free(body_alloc_meta, arr[i]);

		expected_size = size_without_arr - i * (16 + sizeof(t_body_alloc_node) * 2);
		assert_print_size_t(
			body_alloc_meta->max_node_size == expected_size,
			body_alloc_meta->max_node_size,
			expected_size
		);
	}

	body_alloc_free(body_alloc_meta, arr);

	size_t full_memory = buffer_size - sizeof(t_body_alloc_meta) - (sizeof(t_body_alloc_node) * 2);
	assert_print_size_t(
		body_alloc_meta->max_node_size == full_memory,
		body_alloc_meta->max_node_size,
		full_memory
	);

	print_str_literal("[+] Success test body_alloc check max_node_size\n");
}

void test_body_alloc_check_ptr_in_body_alloc_list()
{
	t_body_alloc_meta *body_alloc_meta = (t_body_alloc_meta *)buffer;

	body_alloc_setup(
		body_alloc_meta,
		(char *)buffer + sizeof(t_body_alloc_meta),
		buffer_size - sizeof(t_body_alloc_meta)
	);

	char **arr = body_alloc(body_alloc_meta, sizeof(char *) * 10);
	assert(arr != NULL);

	for (int i = 0; i < 10; i++)
	{
		arr[i] = body_alloc(body_alloc_meta, 100);
		assert(arr[i] != NULL);
		assert(ptr_in_body_alloc_list(body_alloc_meta, arr[i]));
	}

	assert(body_alloc(body_alloc_meta, 20000) == NULL);

	for (int i = 0; i < 10; i++)
	{
		body_alloc_free(body_alloc_meta, arr[i]);
	}
	body_alloc_free(body_alloc_meta, arr);

	assert(!ptr_in_body_alloc_list(body_alloc_meta, (void *)0x01));

	print_str_literal("[+] Success test body_alloc check ptr in body_alloc list\n");
}

void body_alloc_tests()
{
	print_str_literal("Body allocator tests:\n");
	test_body_alloc_allocation();
	test_body_alloc_fail_allocation();
	test_body_alloc_valid_data();
	test_body_alloc_defragmentation();
	test_body_alloc_check_max_node_size();
	test_body_alloc_check_ptr_in_body_alloc_list();
}
