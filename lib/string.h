#ifndef STRING_H
#define STRING_H

#include "../include/types.h"

//memory 
void *memset(void *dst, int c, usize n);
void *memcpy(void *dst, const void *src, usize n);
void *memmove(void *dst, const void *src, usize n);
int   memcmp(const void *a, const void *b, usize n);

//  string 
usize  strlen(const char *s);
int    strcmp(const char *a, const char *b);
int    strncmp(const char *a, const char *b, usize n);
char  *strcpy(char *dst, const char *src);
char  *strncpy(char *dst, const char *src, usize n);

#endif