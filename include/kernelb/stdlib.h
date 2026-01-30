#ifndef _STDLIB_H
#define _STDLIB_H

#include "types.h"

void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
void *calloc(size_t nmemb, size_t size);
void abort(void);
void exit(int status);
int atoi(const char *nptr);
long atol(const char *nptr);

#endif