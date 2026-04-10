/* baSic_ - lib/string.c
 * Copyright (C) 2026 Dhrubo
 * GPL v2 — see LICENSE
 * kernel string and memory utilities, no libc
 */

#include "string.h"

void *memset(void *dst, int c, usize n)
{
    u8 *p = (u8 *)dst;
    while (n--)
        *p++ = (u8)c;
    return dst;
}

void *memcpy(void *dst, const void *src, usize n)
{
    u8       *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;
    while (n--)
        *d++ = *s++;
    return dst;
}

/* safe even when regions overlap */
void *memmove(void *dst, const void *src, usize n)
{
    u8       *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;

    if (d < s) {
        while (n--)
            *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--)
            *--d = *--s;
    }
    return dst;
}

int memcmp(const void *a, const void *b, usize n)
{
    const u8 *pa = (const u8 *)a;
    const u8 *pb = (const u8 *)b;
    while (n--) {
        if (*pa != *pb)
            return *pa - *pb;
        pa++; pb++;
    }
    return 0;
}

usize strlen(const char *s)
{
    usize len = 0;
    while (*s++)
        len++;
    return len;
}

int strcmp(const char *a, const char *b)
{
    while (*a && (*a == *b)) {
        a++; b++;
    }
    return (u8)*a - (u8)*b;
}

int strncmp(const char *a, const char *b, usize n)
{
    while (n && *a && (*a == *b)) {
        a++; b++; n--;
    }
    if (n == 0) return 0;
    return (u8)*a - (u8)*b;
}

char *strcpy(char *dst, const char *src)
{
    char *ret = dst;
    while ((*dst++ = *src++))
        ;
    return ret;
}

char *strncpy(char *dst, const char *src, usize n)
{
    char *ret = dst;
    while (n && (*dst++ = *src++))
        n--;
    /* pad remaining bytes with null if src was shorter */
    while (n--)
        *dst++ = '\0';
    return ret;
}