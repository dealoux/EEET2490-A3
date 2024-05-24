#include "string.h"

int strcmp(const char *str1, const char *str2)
{
    while (*str1 && (*str1 == *str2))
    {
        str1++;
        str2++;
    }
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}

char *strcpy(char *dest, const char *src)
{
    char *original_dest = dest;

    while (*src)
    {
        *dest = *src;
        dest++;
        src++;
    }

    *dest = '\0';

    return original_dest;
}

size_t strlen(const char *str)
{
    const char *s = str;

    while (*s)
    {
        s++;
    }

    return s - str;
}

char *strtok(char *str, const char *delim)
{
    static char *next_token = NULL;
    if (str)
    {
        next_token = str;
    }

    if (!next_token)
    {
        return NULL;
    }

    char *start = next_token;
    while (*next_token)
    {
        const char *d = delim;
        while (*d)
        {
            if (*next_token == *d)
            {
                // Delimiter found
                *next_token = '\0';
                next_token++;
                return start;
            }
            d++;
        }
        next_token++;
    }

    if (start == next_token)
    {
        return NULL;
    }
    return start;
}

char *strstr(const char *haystack, const char *needle)
{
    if (!*needle)
    {
        // If the needle is an empty string, return the whole haystack.
        return (char *)haystack;
    }

    while (*haystack)
    {
        const char *h = haystack;
        const char *n = needle;

        // Loop while characters of h and n are same.
        while (*h && *n && (*h == *n))
        {
            h++;
            n++;
        }

        // If we reached the end of the needle, that means we've found a match.
        if (!*n)
        {
            return (char *)haystack;
        }

        // Move to the next character in the haystack.
        haystack++;
    }

    return NULL;
}
