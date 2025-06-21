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
#include <stdbool.h>

#include "touchpad.h"
#include "util.h"

#define EVENT_DIR "/dev/input/"
#define EVENT_FILE_PREFIX "event"

#define EVENT_TIME_MILLI(event) (event.input_event_sec*1000L\
								 +event.input_event_usec/1000)

#define TOUCH_CODE BTN_TOUCH
#define PRESS_CODE BTN_MOUSE

// delay max between to touch
// to be a double tap
#define DOUBLE_TAP_TIME 250

struct occured_events {
	// values are ignored if < 0
	int x;
	int y;
	int touched;
	int pressed;
	unsigned long touch_time;
};

struct touchpad_struct {
	touchpad_settings_t settings;
	int fd;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	unsigned long last_touch_time;
	struct occured_events occured;
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

#define GET_BIT(var, bit) (var&(1<<bit))
#define SET_BIT(var, bit, val) if (val) var |= (1<<bit); else var &= ~(1<<bit);

#define TR_HAS_NAME_IN_TOUCHPAD(tr) GET_BIT((tr).flags, 0)
#define TR_HAS_ABS(tr) GET_BIT((tr).flags, 1)
#define TR_HAS_XY(tr) GET_BIT((tr).flags, 2)
#define TR_HAS_MT(tr) GET_BIT((tr).flags, 3)
#define TR_HAS_KEY(tr) GET_BIT((tr).flags, 4)
#define TR_HAS_TOUCH(tr) GET_BIT((tr).flags, 5)
#define TR_HAS_PRESS(tr) GET_BIT((tr).flags, 6)

#define TR_SET_NAME_IN_TOUCHPAD(tr, v) SET_BIT((tr).flags, 0, v)
#define TR_SET_ABS(tr, v) SET_BIT((tr).flags, 1, v)
#define TR_SET_XY(tr, v) SET_BIT((tr).flags, 2, v)
#define TR_SET_MT(tr, v) SET_BIT((tr).flags, 3, v)
#define TR_SET_KEY(tr, v) SET_BIT((tr).flags, 4, v)
#define TR_SET_TOUCH(tr, v) SET_BIT((tr).flags, 5, v)
#define TR_SET_PRESS(tr, v) SET_BIT((tr).flags, 6, v)

#define TR_GET_MARK(tr) ((2*TR_HAS_NAME_IN_TOUCHPAD(tr)\
						  +2*TR_HAS_XY(tr)+TR_HAS_MT(tr)+2*TR_HAS_PRESS(tr))\
						  *(TR_HAS_ABS(tr) && (TR_HAS_XY(tr) || TR_HAS_MT(tr))\
							&& (TR_HAS_KEY(tr) && TR_HAS_TOUCH(tr))))

/**
 * Get a feedback on the file descriptor to know
 * if it looks like a touchpad
 */
static void get_touchpad_resemblance(int fd, touchpad_resemblance_t *tr) {
	uint8_t evbit[(EV_CNT+7)/8] = {};
	uint8_t absbit[(ABS_CNT+7)/8] = {};
	uint8_t keybit[(KEY_CNT+7)/8] = {};
	
	exitif(ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) == -1, "ioctl evbit");
	exitif(ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit) == -1, "ioctl absbit");
	exitif(ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) == -1, "ioctl keybit");
	exitif(ioctl(fd, EVIOCGNAME(sizeof(tr->name)), tr->name) == -1, "ioctl name");
	
	TR_SET_NAME_IN_TOUCHPAD(*tr, strstr(tr->name, "Touchpad"));
	
	TR_SET_ABS(*tr, evbit[EV_ABS/8]&(1<<(EV_ABS%8)));
	TR_SET_MT(*tr, absbit[ABS_MT_POSITION_X/8]&(1<<(ABS_MT_POSITION_X%8)));
	TR_SET_XY(*tr, absbit[ABS_X/8]&(1<<(ABS_X%8)) && absbit[ABS_Y/8]&(1<<(ABS_Y%8)));
	
	TR_SET_KEY(*tr, evbit[EV_KEY/8]&(1<<(EV_KEY%8)));
	TR_SET_TOUCH(*tr, keybit[TOUCH_CODE/8]&(1<<(TOUCH_CODE%8)));
	TR_SET_PRESS(*tr, keybit[PRESS_CODE/8]&(1<<(PRESS_CODE%8)));
}

static void print_touchpad_resemblance(touchpad_resemblance_t *tr) {
	printf("%s:\n", tr->name);
	if (TR_HAS_ABS(*tr)) printf(" - support absolute values events\n");
	if (TR_HAS_XY(*tr)) printf(" - support x/y absolute values events\n");
	if (TR_HAS_MT(*tr)) printf(" - support mutli-touch protocol\n");
	if (TR_HAS_KEY(*tr)) printf(" - support key events\n");
	if (TR_HAS_TOUCH(*tr)) printf(" - support touch events\n");
	if (TR_HAS_PRESS(*tr)) printf(" - support press events\n");
}

/**
 * Return a file descriptor describing the touchpad
 * 
 * if device_name is not null, it will search for the best device
 * with this name. Otherwise it will try to found the device
 * that look the most like a touchpad.
 * 
 * list can be LIST_NO, LIST_CANDIDATES or LIST_ALL
 *
 * Return the file descriptor on success, and -1 if no such device
 * are found
 */
static int get_touchpad(char *device_name, int list) {
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
		
		if (list == LIST_ALL || (list == LIST_CANDIDATES && mark)) {
			print_touchpad_resemblance(&tr);
		}
		
		bool names_equal = device_name && !strcmp(device_name, tr.name);
		// To select a device with the correct name even if it is not a touchpad
		if (names_equal) ++mark;
		if ((!device_name || names_equal) && best_mark < mark) {
			strcpy(best_path, path);
			best_tr = tr;
			best_mark = mark;
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
	
	bool device_ok = true;
	printf("Found device: %s\n", best_tr.name);
	printf("on %s\n", best_path);
	if (!TR_HAS_NAME_IN_TOUCHPAD(best_tr) && !device_name) {
		fprintf(stderr, "Warning: found device don't have \"Touchpad\" in it's name\n");
	}
	if (!TR_HAS_ABS(best_tr)) {
		fprintf(stderr, "Error: found device don't support absolute values events\n");
		device_ok = false;
	}
	if (!TR_HAS_XY(best_tr)) {
		fprintf(stderr, "Error: found device don't support asbolute x/y events\n");
		if (TR_HAS_MT(best_tr))
			fprintf(stderr, "This program does not support multi-touch protocol yet\n");
		device_ok = false;
	}
	if (!TR_HAS_KEY(best_tr)) {
		fprintf(stderr, "Error: found device don't support key events. "
				"Key events are used to detect when the touchpad "
				"is touched or pressed\n");
		device_ok = false;
	}
	if (!TR_HAS_TOUCH(best_tr)) {
		fprintf(stderr, "Error: found device don't support touch events\n");
		//device_ok = false;
	}
	if (!TR_HAS_PRESS(best_tr)) {
		fprintf(stderr, "Warning: found device don't support press events\n");
	}
	
	fd = open(best_path, O_RDONLY);
	exitif(fd == -1, "cannot open %s", best_path);
	return (device_ok? fd: -1);
}

static void reset_occured_events(struct occured_events *evt) {
	evt->x = -1;
	evt->y = -1;
	evt->touched = -1;
	evt->pressed = -1;
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
	int fd = get_touchpad(settings->device_name, settings->list);
	if (fd < 0) return -1;
	
	touch_st.fd = fd;
	touch_st.settings = *settings;
	reset_occured_events(&touch_st.occured);
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

static void applie_occured_events(struct occured_events *evt) {
	pthread_mutex_lock(&touch_st.mutex);
	if (evt->x >= 0) {
		touch_st.info.x = evt->x;
		if (evt->x <= touch_st.settings.minx)
			touch_st.info.edgex = -1;
		else if (evt->x >= touch_st.settings.maxx)
			touch_st.info.edgex = 1;
		else touch_st.info.edgex = 0;
	}
	if (evt->y >= 0) {
		touch_st.info.y = evt->y;
		if (evt->y <= touch_st.settings.miny)
			touch_st.info.edgey = -1;
		else if (evt->y >= touch_st.settings.maxy)
			touch_st.info.edgey = 1;
		else touch_st.info.edgey = 0;
	}
	
	if (evt->touched >= 0 &&
		(evt->touched == 0 || dont_touch_borders()
		 || touch_st.settings.no_edge_protection)) {
		touch_st.info.touched = evt->touched;
		if (evt->touched == 0) touch_st.info.double_tapped = 0;
		if (evt->touched > 0) {
			if (evt->touch_time-touch_st.last_touch_time < DOUBLE_TAP_TIME) {
				// double tap detected
				touch_st.info.double_tapped = 1;
			}
			touch_st.last_touch_time = evt->touch_time;
		}
	}
	
	if (evt->pressed >= 0) touch_st.info.pressed = evt->pressed;
	pthread_mutex_unlock(&touch_st.mutex);
	
	if (evt->touched > 0 || evt->pressed > 0)
		pthread_cond_signal(&touch_st.cond);
}

void touchpad_read_next_event() {
	struct input_event event = {};
	
	exitif(read(touch_st.fd, &event, sizeof(event)) == -1,
		   "cannot read from the touchpad event file");
	//printf("%d %d %d\n", event.type, event.code, event.value);
	if (event.type == EV_SYN) {
		applie_occured_events(&touch_st.occured);
		reset_occured_events(&touch_st.occured);
	} else if (event.type == EV_KEY) {
		switch (event.code) {
		case TOUCH_CODE: // touched
			touch_st.occured.touched = event.value;
			if (event.value)
				touch_st.occured.touch_time = EVENT_TIME_MILLI(event);
			break;
		case PRESS_CODE: // pressed
			touch_st.occured.pressed = event.value;
			break;
		}
	} else if (event.type == EV_ABS) {
		// moved
		switch (event.code) {
		case ABS_X:
			touch_st.occured.x = event.value;
			break;
		case ABS_Y:
			touch_st.occured.y = event.value;
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
