#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdbool.h>

/**
 * Print an error message with the format
 * kerpad: message
 */
void error_message(const char *message, ...);

/**
 * Print an error message if the condition is true
 * use perror if errno != 0
 *
 * return the condition
 */
bool msgif(bool condition, const char *prefix, ...);

/**
 * Print an error message and exit if the condition is true
 * use perror if errno != 0
 */
void exitif(bool condition, const char *prefix, ...);

#endif // !__UTIL_H__
