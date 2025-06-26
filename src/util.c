#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include "util.h"

#define PROG_NAME "kerpad"

void error_message(const char *message, ...) {
	char format_message[255] = PROG_NAME": ";
	va_list argptr;
	va_start(argptr, message);
	vsprintf(format_message+strlen(format_message), message, argptr);
	va_end(argptr);
	fprintf(stderr, format_message);
}

void exitif(bool condition, const char *prefix, ...) {
	if (condition) {
		char format_prefix[255] = PROG_NAME": ";
		va_list argptr;
		va_start(argptr, prefix);
		vsprintf(format_prefix+strlen(format_prefix), prefix, argptr);
		va_end(argptr);
		if (errno != 0) {
			perror(format_prefix);
		} else {
			fprintf(stderr, "%s\n", format_prefix);
		}
		exit(EXIT_FAILURE);
	}
}
