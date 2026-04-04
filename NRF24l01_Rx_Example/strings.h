#ifndef STRINGS_H_
#define STRINGS_H_

#include <stdint.h>
#include <stdbool.h>

bool areStringsEqual(char *s1, char *s2);
int findLen(const char *s);
// Int To String
char *itoa(uint32_t pid, char *str);



#endif
