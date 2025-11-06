#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"

void clear_input_buffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

// Trim leading and trailing whitespace, newlines, carriage returns, and special characters
char *trim(char *str)
{
    if (!str)
        return str;

    // Trim leading whitespace and special characters (\n, \r, \t, space, etc.)
    while (*str && (isspace((unsigned char)*str) || *str == '\n' || *str == '\r' || *str == '\t'))
        str++;

    // Trim trailing whitespace and special characters
    char *end = str + strlen(str) - 1;
    while (end > str && (isspace((unsigned char)*end) || *end == '\n' || *end == '\r' || *end == '\t'))
        *end-- = '\0';

    return str;
}