#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static inline void exitif(int condition, const char *prefix) {
	if (condition) {
		if (errno != 0) {
			perror(prefix);
		} else {
			fprintf(stderr, "%s\n", prefix);
		}
		exit(1);
	}
}

#endif // !__UTIL_H__
