#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stdlib.h>

#define da_append(xs, x)\
	do {\
		if (xs.count >= xs.capacity) {\
			if (xs.capacity == 0) xs.capacity = 4;\
			else xs.capacity *= 2;\
			xs.items = realloc(xs.items, xs.capacity*sizeof(*xs.items));\
		}\
		xs.items[xs.count++] = x;\
	} while(0)
	
#endif