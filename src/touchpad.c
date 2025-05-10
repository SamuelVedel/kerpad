/*
 * This file is responsible of retrieving
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

#include "touchpad.h"
#include "util.h"

#define EVENT_DIR "/dev/input/"
#define EVENT_FILE_PREFIX "event"

#define EVENT_TIME_MILLI(event) (event.input_event_sec*1000L \
								 +event.input_event_usec/1000)

// delay max between to touch
// to be a double touch
#define DOUBLE_TOUCH_TIME 250

struct touchpad_struct {
	int fd;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	unsigned long last_touch_time;
	touchpad_info_t info;
};

struct touchpad_struct touch_st = {
	.fd = -1,
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.cond = PTHREAD_COND_INITIALIZER
};


/**
 * Return a value different from 0 if
 * this file descriptor correspond to
 * a touchpad event file
 */
int is_touchpad(int fd) {
	char name[256] = {};
	ioctl(fd, EVIOCGNAME(sizeof(name)), name);
	
	if (strstr(name, "Touchpad"))
		return 1;
	
	return 0;
}

int touchpad_init() {
	DIR *dir = opendir(EVENT_DIR);
	exitif(dir == NULL, "openning event directory");
	struct dirent *de;
	int prefix_len = strlen(EVENT_FILE_PREFIX);
	int fd = -1;
	
	while ((de = readdir(dir))) {
		exitif(errno != 0, "reading event directory");
		
		if (strncmp(de->d_name, EVENT_FILE_PREFIX, prefix_len)) continue;
		char path[100];
		strcpy(path, EVENT_DIR);
		strcat(path, de->d_name);
		
		fd = open(path, O_RDONLY);
		exitif(fd == -1, "opening an event file");
		
		if (is_touchpad(fd)) break;
		
		exitif(close(fd) == -1, "closing an event file");
		fd = -1;
	}
	
	exitif(closedir(dir) == -1, "clossing the event directory");
	if (fd == -1) return -1;
	
	touch_st.fd = fd;
	return 0;
}

void touchpad_read_next_event() {
	struct input_event event = {};
	
	exitif(read(touch_st.fd, &event, sizeof(event)) == -1,
		   "reading from the touchpad event file");
	if (event.type == EV_KEY){
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
				if (touch_time-touch_st.last_touch_time < DOUBLE_TOUCH_TIME) {
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
		switch (event.code) {
		case REL_X:
			pthread_mutex_lock(&touch_st.mutex);
			touch_st.info.x = event.value;
			pthread_mutex_unlock(&touch_st.mutex);
			break;
		case REL_Y:
			pthread_mutex_lock(&touch_st.mutex);
			touch_st.info.y = event.value;
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
	exitif(close(touch_st.fd) == -1, "closing the touchpad event file");
}
