/*
 * This file is responsible for retrieving
 * touchpad coordinates
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>

#include "touchpad.h"
#include "util.h"

#define EVENT_DIR "/dev/input/"
#define EVENT_FILE_PREFIX "event"

#define EVENT_TIME_MILLI(event) (event.input_event_sec*1000L\
								 +event.input_event_usec/1000)

// delay max between to touch
// to be a double touch
#define DOUBLE_TOUCH_TIME 250

struct touchpad_struct {
	touchpad_settings_t settings;
	int fd;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	unsigned long last_touch_time;
	touchpad_info_t info;
};

static struct touchpad_struct touch_st = {
	.fd = -1,
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.cond = PTHREAD_COND_INITIALIZER
};

struct touchpad_resemblance {
	char name[255];
	uint8_t flags;
};
typedef struct touchpad_resemblance touchpad_resemblance_t;

#define TR_HAS_NAME_IN_TOUCHPAD(tr) ((tr).flags&1)
#define TR_HAS_ABS(tr) ((tr).flags&(1<<1))
#define TR_HAS_XY(tr) ((tr).flags&(1<<2))
#define TR_HAS_MT(tr) ((tr).flags&(1<<3))

#define TR_SET_NAME_IN_TOUCHPAD(tr, v) if (v) (tr).flags |= 1; else (tr).flags &= ~1;
#define TR_SET_ABS(tr, v) if (v) (tr).flags |= (1<<1); else (tr).flags &= ~(1<<1);
#define TR_SET_XY(tr, v) if (v) (tr).flags |= (1<<2); else (tr).flags &= ~(1<<2);
#define TR_SET_MT(tr, v) if (v) (tr).flags |= (1<<3); else (tr).flags &= ~(1<<3);

#define TR_GET_MARK(tr) ((TR_HAS_NAME_IN_TOUCHPAD(tr)*2\
						  +TR_HAS_XY(tr)*2+TR_HAS_MT(tr))\
						  *(TR_HAS_ABS(tr) && (TR_HAS_XY(tr) || TR_HAS_MT(tr))))

/**
 * Get a feedback on the file descriptor to know
 * if it looks like a touchpad
 */
static void get_touchpad_resemblance(int fd, touchpad_resemblance_t *tr) {
	unsigned long evbit[(EV_CNT+7)/8] = {};
	unsigned long absbit[(ABS_CNT+7)/8] = {};
	
	exitif(ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) == -1, "ioctl evbit");
	exitif(ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit) == -1, "ioctl absbit");
	exitif(ioctl(fd, EVIOCGNAME(sizeof(tr->name)), tr->name) == -1, "ioctl name");
	
	TR_SET_NAME_IN_TOUCHPAD(*tr, strstr(tr->name, "Touchpad"));
	
	TR_SET_ABS(*tr, evbit[EV_ABS/8]&(1<<(EV_ABS%8)));
	TR_SET_MT(*tr, absbit[ABS_MT_POSITION_X/8]&(1<<(ABS_MT_POSITION_X%8)));
	TR_SET_XY(*tr, absbit[ABS_X/8]&(1<<(ABS_X%8)) && absbit[ABS_Y/8]&(1<<(ABS_Y%8)));
}

/**
 * Return a file descriptor describing the touchpad
 * if device_name is not null, it will search for a device
 * with this name. Otherwise it will try to found the device
 * that look like a touchpad the most.
 *
 * Return the file descriptor on success, and -1 if no such device
 * are found
 */
static int get_touchpad(char *device_name) {
	DIR *dir = opendir(EVENT_DIR);
	exitif(dir == NULL, "cannot open %s directory", EVENT_DIR);
	struct dirent *de;
	int prefix_len = strlen(EVENT_FILE_PREFIX);
	int fd = -1;
	
	char best_path[255] = {};
	touchpad_resemblance_t best_tr = {};
	int best_mark = 0;
	
	while ((de = readdir(dir))) {
		exitif(errno != 0, "cannot read from %s directory", EVENT_DIR);
		
		if (strncmp(de->d_name, EVENT_FILE_PREFIX, prefix_len)) continue;
		char path[255];
		strcpy(path, EVENT_DIR);
		strcat(path, de->d_name);
		
		fd = open(path, O_RDONLY);
		exitif(fd == -1, "cannot open %s", path);
		
		touchpad_resemblance_t tr = {};
		get_touchpad_resemblance(fd, &tr);
		int mark = TR_GET_MARK(tr);
		bool names_equal = device_name && !strcmp(device_name, tr.name);
		if ((!device_name && best_mark < mark) || names_equal) {
			strcpy(best_path, path);
			best_tr = tr;
			best_mark = 1;
			if (names_equal) {
				exitif(close(fd) == -1, "cannot close %s", path);
				fd = -1;
				break;
			}
		}
		
		exitif(close(fd) == -1, "cannot close %s", path);
		fd = -1;
	}
	exitif(closedir(dir) == -1, "cannot close %s directory", EVENT_DIR);
	
	if (best_mark == 0) {
		if (!device_name) fprintf(stderr, "No touchpad found\n");
		else fprintf(stderr, "No device named %s found\n", device_name);
		return -1;
	}
	
	printf("Found device: %s\n", best_tr.name);
	printf("on %s\n", best_path);
	if (!TR_HAS_NAME_IN_TOUCHPAD(best_tr) && !device_name) {
		fprintf(stderr, "Warning: found device don't have \"Touchpad\" in it's name\n");
	}
	if (!TR_HAS_ABS(best_tr)) {
		fprintf(stderr, "Error: found device don't support absolute values events\n");
		return -1;
	}
	if (!TR_HAS_XY(best_tr)) {
		fprintf(stderr, "Error: found device don't support asbolute x/y events\n");
		if (TR_HAS_MT(best_tr))
			fprintf(stderr, "This program does not support multi-touch protocol yet\n");
		return -1;
	}
	
	fd = open(best_path, O_RDONLY);
	exitif(fd == -1, "cannot open %s", best_path);
	return fd;
}

static void init_edge_limits() {
	struct input_absinfo xlimits = {};
	exitif(ioctl(touch_st.fd, EVIOCGABS(ABS_X), &xlimits) == -1, "ioctl get x limits");
	struct input_absinfo ylimits = {};
	exitif(ioctl(touch_st.fd, EVIOCGABS(ABS_Y), &ylimits) == -1, "ioctl get y limits");
	
	touchpad_settings_t *ts = &touch_st.settings;
	if (ts->edge_thickness < 0) ts->edge_thickness = DEFAULT_EDGE_THICKNESS;
	if (ts->minx < 0) ts->minx = xlimits.minimum+ts->edge_thickness;
	if (ts->maxx < 0) ts->maxx = xlimits.maximum-ts->edge_thickness;
	if (ts->miny < 0) ts->miny = ylimits.minimum+ts->edge_thickness;
	if (ts->maxy < 0) ts->maxy = ylimits.maximum-ts->edge_thickness;
}

int touchpad_init(touchpad_settings_t *settings) {
	int fd = get_touchpad(settings->device_name);
	if (fd < 0) return -1;
	
	touch_st.fd = fd;
	touch_st.settings = *settings;
	init_edge_limits();
	return 0;
}

/**
 * Return false if the touchpad coordinates are
 * beyond the borders defined in the settings
 */
static int dont_touch_borders() {
	int x = touch_st.info.x;
	int y = touch_st.info.y;
	return x >= touch_st.settings.minx
		&& x <= touch_st.settings.maxx
		&& y >= touch_st.settings.miny
		&& y <= touch_st.settings.maxy;
}

void touchpad_read_next_event() {
	struct input_event event = {};
	
	exitif(read(touch_st.fd, &event, sizeof(event)) == -1,
		   "cannot read from the touchpad event file");
	if (event.type == EV_KEY) {
		//printf("%d %d %d\n", event.type, event.code, event.value);
		switch (event.code) {
		case 330: // touched
			pthread_mutex_lock(&touch_st.mutex);
			touch_st.info.touching = event.value;
			if (!event.value) touch_st.info.double_touching = 0;
			pthread_mutex_unlock(&touch_st.mutex);
			
			// save touch time
			// and check for double touch
			if (event.value) {
				unsigned long touch_time = EVENT_TIME_MILLI(event);
				if (touch_time-touch_st.last_touch_time < DOUBLE_TOUCH_TIME
					&& dont_touch_borders()) {
					// double touch detected
					pthread_mutex_lock(&touch_st.mutex);
					touch_st.info.double_touching = 1;
					pthread_mutex_unlock(&touch_st.mutex);
				}
				touch_st.last_touch_time = touch_time;
				pthread_cond_signal(&touch_st.cond);
			}
			break;
		case 272: // pressed
			pthread_mutex_lock(&touch_st.mutex);
			touch_st.info.pressing = event.value;
			pthread_mutex_unlock(&touch_st.mutex);
			
			if (event.value)
				pthread_cond_signal(&touch_st.cond);
			break;
		}
	} else if (event.type == EV_ABS) {
		// moved
		switch (event.code) {
		case ABS_X:
			pthread_mutex_lock(&touch_st.mutex);
			touch_st.info.x = event.value;
			if (event.value <= touch_st.settings.minx)
				touch_st.info.edgex = -1;
			else if (event.value >= touch_st.settings.maxx)
				touch_st.info.edgex = 1;
			else touch_st.info.edgex = 0;
			pthread_mutex_unlock(&touch_st.mutex);
			break;
		case ABS_Y:
			pthread_mutex_lock(&touch_st.mutex);
			touch_st.info.y = event.value;
			if (event.value <= touch_st.settings.miny)
				touch_st.info.edgey = -1;
			else if (event.value >= touch_st.settings.maxy)
				touch_st.info.edgey = 1;
			else touch_st.info.edgey = 0;
			pthread_mutex_unlock(&touch_st.mutex);
			break;
		}
	}
}

void touchpad_get_info(touchpad_info_t *info) {
	pthread_mutex_lock(&touch_st.mutex);
	*info = touch_st.info;
	pthread_mutex_unlock(&touch_st.mutex);
}

void touchpad_wait() {
	pthread_mutex_lock(&touch_st.mutex);
	pthread_cond_wait(&touch_st.cond, &touch_st.mutex);
	pthread_mutex_unlock(&touch_st.mutex);
}

void touchpad_signal() {
	pthread_cond_signal(&touch_st.cond);
}

void touchpad_clean() {
	if (touch_st.fd == -1) return;
	exitif(close(touch_st.fd) == -1, "cannot close the touchpad event file");
}
