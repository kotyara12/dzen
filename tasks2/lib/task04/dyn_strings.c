#include "dyn_strings.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

char * malloc_string(const char *source) 
{
  if (source) {
    uint32_t len = strlen(source);
    char *ret = (char*)malloc(len+1);
    if (ret != NULL) {
      memset(ret, 0, len+1);
      strcpy(ret, source);
      return ret;
    }
  };
  return NULL;
}

char * malloc_stringf(const char *format, ...) 
{
  char *ret = NULL;
  if (format != NULL) {
    // get the list of arguments
    va_list args1, args2;
    va_start(args1, format);
    va_copy(args2, args1);
    // calculate length of resulting string
    int len = vsnprintf(NULL, 0, format, args1);
    va_end(args1);
    // allocate memory for string
    if (len > 0) {
      ret = (char*)malloc(len+1);
      if (ret != NULL) {
        memset(ret, 0, len+1);
        vsnprintf(ret, len+1, format, args2);
      };
    };
    va_end(args2);
  };
  return ret;
}

