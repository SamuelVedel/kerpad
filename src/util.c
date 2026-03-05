#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include "util.h"

#define PROG_NAME "kerpad"

#define FORMAT_MSG(msg, formated_msg) \
	char formated_msg[255] = PROG_NAME": ";\
	va_list argptr;\
	va_start(argptr, msg);\
	vsprintf(formated_msg+strlen(formated_msg), msg, argptr);\
	va_end(argptr);

void error_message(const char *message, ...) {
	FORMAT_MSG(message, formated_message);
	fprintf(stderr, formated_message);
}

bool msgif(bool condition, const char *prefix, ...) {
	if (condition) {
		FORMAT_MSG(prefix, formated_prefix);
		if (errno != 0) {
			perror(formated_prefix);
		} else {
			fprintf(stderr, "%s\n", formated_prefix);
		}
	}
	return condition;
}

void exitif(bool condition, const char *prefix, ...) {
	if (condition) {
		FORMAT_MSG(prefix, formated_prefix);
		msgif(condition, formated_prefix);
		exit(EXIT_FAILURE);
	}
}
