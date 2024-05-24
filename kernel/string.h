#ifndef STRING_H
#define STRING_H
#include "../gcclib/stddef.h"
#include "../gcclib/stdint.h"
#include "../gcclib/stdarg.h"
int strcmp(const char *str1, const char *str2);
char *strcpy(char *dest, const char *src);
char *strtok(char *str, const char *delim);
size_t strlen(const char *str);
char *strstr(const char *haystack, const char *needle);
#endif