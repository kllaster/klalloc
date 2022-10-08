#include "utils.h"

size_t align(size_t n)
{
	return (n + sizeof(t_word) - 1) & ~(sizeof(t_word) - 1);
}