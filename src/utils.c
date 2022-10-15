#include "utils.h"

inline size_t align(size_t n, size_t round_up_to)
{
	return (n + round_up_to - 1) & ~(round_up_to - 1);
}

inline void print_ptr(const void *ptr)
{
	print_str_literal("0x");
	print_num_base((size_t)ptr, 16, 9);
}

inline void print_num(size_t num)
{
	print_num_base(num, 10, 0);
}

void print_num_base(size_t num, size_t base, size_t width)
{
	static const char base_str[] = "0123456789abcdefghijklmnopqrstuvwxyz";

	if (num / base)
		print_num_base(num / base, base, width != 0 ? width - 1 : 0);
	else if (width)
	{
		while (--width > 0)
			write(STDIN_FILENO, "0", 1);
	}
	write(STDIN_FILENO, &base_str[num % base], 1);
}