/*
 * This file is responsible for
 * simulating a mouse
 */

#include <stdlib.h>
#include <linux/uinput.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "mouse.h"
#include "util.h"

struct mouse {
	int ui_fd;
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
	
	usetup.id.bustype = BUS_USB;
	usetup.id.vendor = 0x1234;
	usetup.id.product = 0x5678;
	strcpy(usetup.name, name);
	ioctl(mouse->ui_fd, UI_DEV_SETUP, &usetup);
	ioctl(mouse->ui_fd, UI_DEV_CREATE);
	sleep(1);
	
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
	mouse_emit(mouse, EV_REL, REL_X, dx);
	mouse_emit(mouse, EV_REL, REL_Y, dy);
	mouse_emit(mouse, EV_SYN, SYN_REPORT, 0);
}

void mouse_move_x(mouse_t *mouse, int dx) {
	mouse_emit(mouse, EV_REL, REL_X, dx);
	mouse_emit(mouse, EV_SYN, SYN_REPORT, 0);
}

void mouse_move_y(mouse_t *mouse, int dy) {
	mouse_emit(mouse, EV_REL, REL_Y, dy);
	mouse_emit(mouse, EV_SYN, SYN_REPORT, 0);
}

void mouse_clean(mouse_t *mouse) {
	sleep(1);
	ioctl(mouse->ui_fd, UI_DEV_DESTROY);
	close(mouse->ui_fd);
	free(mouse);
}
