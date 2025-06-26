#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdbool.h>

void error_message(const char *message, ...);

void exitif(bool condition, const char *prefix, ...);

#endif // !__UTIL_H__
