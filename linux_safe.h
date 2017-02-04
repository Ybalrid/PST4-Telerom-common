#pragma once
#ifdef __GNUC__

#include <cstring>
#include <algorithm>

using rsize_t = size_t;
using errno_t = int;

//TODO define RSIZE_MAX_STR to some value

static errno_t strcpy_s(char* dest, rsize_t dmax, const char* src)
{
	if (dest == nullptr) return 1;
	if (dmax == 0) return 2;
	//TODO if(dmax > RSIZE_MAX_STR) return 3;
	if (src == nullptr) { dest[0] = '\0'; return 4; }
	if (src == dest) return 5;

	const size_t stringSize = strlen(src) + 1;
	size_t size{ std::min(stringSize, dmax) };

	memcpy(dest, src, size);

	for (auto i{ stringSize }; i < dmax; i++)
		dest[i] = '\0';

	return 0;
}

#endif
