#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "vector.h"

int vector_grow_many(Vector *v, size_t n)           //function to increase the number of pools in vector
{
	/* check for maximum number of pools */
	if (v->nr_pool + n > MAX_SZ/v->pool) return -ENOMEM;
	void *p = malloc((v->nr_pool + n) * v->pool);
	if (!p) return -errno;
	memcpy(p, v->p, v->unit * v->length);
	free(v->p);
	v->p = p;
	v->nr_pool+= n;
	return 0;
}

int vector_shrink_to_fit(Vector *v)             //function to shrink to exact size of vector
{
	// empty vector
	if (!v->pool) return 0;
	size_t used = v->unit * v->length;
	if (used % v->pool) return 0;
	/* stored size is multiple of pool size, find if a pool is free */
	size_t diff = v->nr_pool - used / v->pool;
	assert(diff >= 0);
	if (diff) {
		unsigned char *b;
		v->nr_pool -= diff;
		b = realloc(v->p, (v->nr_pool) * (v->pool));
		if (!b) return -errno;
		v->p = b;
	} /* vector partially pins a pool, hence no shrinking can be done */
	return 0;
}

int vector_insert(Vector *v, size_t pos, const void *p)         //function to insert 1 element in vector at a given position
{
	return vector_insert_many(v, pos, p, 1);
}

int vector_insert_many(Vector *v, size_t pos, const void *p, size_t n)      //function to insert elements in vector at a given position
{
	assert(v);
	assert(p);
	assert(n);
	/* check for addition overflow */
	if (n > SIZE_MAX - v->length) return -69;
	/* check for max number of elements possible */
	if (v->length + n > MAX_SZ/v->unit) return -ENOMEM;
	if (pos > v->length) return -EINVAL;
	if (v->nr_pool * v->pool < v->unit * (v->length + n)) {
		int r = vector_grow_many(v, 1);
		if (r < 0) return r;
	}
	/* pointer to where we insert the element */
	unsigned char *i = v->p + v->unit * pos;
	/* find remaining buffer length */
	size_t rem = v->length - pos;
	if (rem) memmove(i + v->unit * n, i, v->unit * rem);
	memcpy(i, p, v->unit * n);
	v->length += n;
	return 0;
}

int vector_push_back(Vector *v, const void *p)              //function to push new element into vector
{
	return vector_insert_many(v, v->length, p, 1);
}

bool vector_pop_back(Vector *v)             //function to pop element from vector
{
	assert(v);
	if (!v->length) return false;
	v->length--;
	if (!v->length)	vector_reset(v);
	return true;
}

Vector* vector_new(size_t unit)             //function to create a new vector
{
	if (!unit) return NULL;
	Vector *v = malloc(sizeof(*v));
	if (!v)	return NULL;
	v->length = 0;
	v->unit = unit;
	if (unit > SIZE_MAX/32) v->pool = unit;
	else v->pool = v->unit * 32;
	v->p = malloc(v->pool);
	if (!v->p) {
		free(v);
		return NULL;
	}
	v->nr_pool = 1;
	return v;
}

void vector_reset(Vector *v)        //function reseting all members in vector
{
	v->length = 0;
	v->unit = 0;
	v->nr_pool = 0;
	v->pool = 0;
	free(v->p);
	v->p = NULL;
}
