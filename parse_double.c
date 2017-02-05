#include "parse_double.h"
#include <ctype.h>

bool isDouble(const char *s)
{
    int comaCount = 0;

    if(*s != '-' && !isdigit(*s))
        return false;

    s++;

    while(*s)
    {
        if(*s == '.' || *s == ',')
            comaCount ++;
        else if(!isdigit(*s))
            return false;

        s++;
    }

    if(comaCount > 1)
        return false;

    return true;
}

double parseDouble(const char *s)
{
    int sign;
    double value = 0;
    double d = 1.0;

    if(*s == '-')
    {
        sign = -1;
        s++;
    }
    else
        sign = 1;

    while(*s && *s != '.' && *s != ',')
    {
        value = 10 * value + *s - '0';
        s++;
    }

    if(*s)
        s++;

    while(*s)
    {
        value = 10 * value + *s - '0';
        d *= 10;
        s++;
    }

    return value / d * sign;
}
