/*
 * This file is responsible for
 * simulating a mouse
 */

#include <stdlib.h>
#include <linux/uinput.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "mouse.h"
#include "util.h"

struct mouse {
	int ui_fd;
	pthread_mutex_t mutex;
};

mouse_t *mouse_init(const char *name) {
	mouse_t *mouse = malloc(sizeof(*mouse));
	mouse->ui_fd = open("/dev/uinput", O_WRONLY|O_NONBLOCK);
	exitif(mouse->ui_fd == -1, "cannot open /dev/uinput");
	struct uinput_setup usetup = {};
	
	ioctl(mouse->ui_fd, UI_SET_EVBIT, EV_KEY);
	ioctl(mouse->ui_fd, UI_SET_KEYBIT, BTN_LEFT);
	
	ioctl(mouse->ui_fd, UI_SET_EVBIT, EV_REL);
	ioctl(mouse->ui_fd, UI_SET_RELBIT, REL_X);
	ioctl(mouse->ui_fd, UI_SET_RELBIT, REL_Y);
	ioctl(mouse->ui_fd, UI_SET_RELBIT, REL_WHEEL_HI_RES);
	ioctl(mouse->ui_fd, UI_SET_RELBIT, REL_HWHEEL_HI_RES);
	
	usetup.id.bustype = BUS_USB;
	usetup.id.vendor = 0x1234;
	usetup.id.product = 0x5678;
	strcpy(usetup.name, name);
	ioctl(mouse->ui_fd, UI_DEV_SETUP, &usetup);
	ioctl(mouse->ui_fd, UI_DEV_CREATE);
	sleep(1);
	
	pthread_mutex_init(&mouse->mutex, NULL);
	
	return mouse;
}


static void mouse_emit(mouse_t *mouse, int type, int code, int val) {
	struct input_event ie = {
		.type = type,
		.code = code,
		.value = val
	};
	ie.time.tv_sec = 0;
	ie.time.tv_usec = 0;
	
	exitif(write(mouse->ui_fd, &ie, sizeof(ie)) == -1, "cannot write to /dev/uinput");
}

void mouse_move(mouse_t *mouse, int dx, int dy) {
	pthread_mutex_lock(&mouse->mutex);
	mouse_emit(mouse, EV_REL, REL_X, dx);
	mouse_emit(mouse, EV_REL, REL_Y, dy);
	mouse_emit(mouse, EV_SYN, SYN_REPORT, 0);
	pthread_mutex_unlock(&mouse->mutex);
}

void mouse_move_x(mouse_t *mouse, int dx) {
	pthread_mutex_lock(&mouse->mutex);
	mouse_emit(mouse, EV_REL, REL_X, dx);
	mouse_emit(mouse, EV_SYN, SYN_REPORT, 0);
	pthread_mutex_unlock(&mouse->mutex);
}

void mouse_move_y(mouse_t *mouse, int dy) {
	pthread_mutex_lock(&mouse->mutex);
	mouse_emit(mouse, EV_REL, REL_Y, dy);
	mouse_emit(mouse, EV_SYN, SYN_REPORT, 0);
	pthread_mutex_unlock(&mouse->mutex);
}

void mouse_scroll_x(mouse_t *mouse, int dx) {
	pthread_mutex_lock(&mouse->mutex);
	mouse_emit(mouse, EV_REL, REL_HWHEEL_HI_RES, dx);
	mouse_emit(mouse, EV_SYN, SYN_REPORT, 0);
	pthread_mutex_unlock(&mouse->mutex);
}

void mouse_scroll_y(mouse_t *mouse, int dy) {
	pthread_mutex_lock(&mouse->mutex);
	mouse_emit(mouse, EV_REL, REL_WHEEL_HI_RES, dy);
	mouse_emit(mouse, EV_SYN, SYN_REPORT, 0);
	pthread_mutex_unlock(&mouse->mutex);
}

void mouse_clean(mouse_t *mouse) {
	sleep(1);
	ioctl(mouse->ui_fd, UI_DEV_DESTROY);
	close(mouse->ui_fd);
	pthread_mutex_destroy(&mouse->mutex);
	free(mouse);
}
