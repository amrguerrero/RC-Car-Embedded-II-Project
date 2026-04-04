#include <stdint.h>
#include <stdbool.h>
#include "strings.h"

int findLen(const char *s)
{
    int c = 0;

    while (*s++)
        c++;

    return c;
}

bool areStringsEqual(char *s1, char *s2)
{

    uint8_t i;
    // Compare character by character
    for (i = 0; i < findLen(s1); ++i)
    {
        if (s1[i] != s2[i])
        {
            return 0;
        }
    }

    return 1;
}

// Int To String
char *itoa(uint32_t pid, char *str)
{
    int t = 0;
    if (pid == 0)
    {
        str[t] = '0';
        t++;
        str[t] = '\0';
        return str;
    }

    while (pid > 0)
    {
        str[t] = (pid % 10) + '0';
        pid /= 10;
        t++;
    }
    str[t] = '\0';
    int s = 0, e = t - 1;
    while (s < e)
    {
        char temp = str[e];
        str[e] = str[s];
        str[s] = temp;
        s++;
        e--;
    }
    return str;
}
