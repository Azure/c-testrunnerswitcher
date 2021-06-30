// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "c_logging/xlogging.h"

#include "ctrs_sprintf.h"

static char* ctrs_vsprintf_char(const char* format, va_list va)
{
    char* result;
    va_list va_clone;
    va_copy(va_clone, va); /*passing "va" to vsnprintf twice is undefined behavior, so "va" is passed and then "va_clone"*/
    int neededSize = vsnprintf(NULL, 0, format, va);
    if (neededSize < 0)
    {
        LogError("failure in vsnprintf(NULL, 0, format=%s, va=%p)", format, (void*)&va);
        result = NULL;
    }
    else
    {
        result = malloc(neededSize + 1);
        if (result == NULL)
        {
            LogError("failure in malloc(neededSize=%d + 1);", neededSize);
            /*return as is*/
        }
        else
        {
            if (vsnprintf(result, neededSize + 1, format, va_clone) != neededSize)
            {
                LogError("failure in vsnprintf(result, neededSize=%d + 1, format=%s, va_clone=%p) va=%p ", neededSize, format, (void*)&va_clone, (void*)&va);
                free(result);
                result = NULL;
            }
        }
    }
    va_end(va_clone);
    return result;
}

/*returns a char* that is as if printed by printf*/
/*needs to be free'd after usage*/
char* ctrs_sprintf_char(const char* format, ...)
{
    char* result;
    va_list va;
    va_start(va, format);
    result = ctrs_vsprintf_char(format, va);
    va_end(va);
    return result;
}

void ctrs_sprintf_free(char* string)
{
    free(string);
}
