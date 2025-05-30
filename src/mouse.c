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

int ui_fd = -1;

void mouse_init() {
	ui_fd = open("/dev/uinput", O_WRONLY|O_NONBLOCK);
	exitif(ui_fd == -1, "Faile to open /dev/uinput");
	struct uinput_setup usetup = {};
	
	ioctl(ui_fd, UI_SET_EVBIT, EV_KEY);
	ioctl(ui_fd, UI_SET_KEYBIT, BTN_LEFT);
	
	ioctl(ui_fd, UI_SET_EVBIT, EV_REL);
	ioctl(ui_fd, UI_SET_RELBIT, REL_X);
	ioctl(ui_fd, UI_SET_RELBIT, REL_Y);
	
	usetup.id.bustype = BUS_USB;
	usetup.id.vendor = 0x1234;
	usetup.id.product = 0x5678;
	strcpy(usetup.name, "Kerpad Mouse");
	ioctl(ui_fd, UI_DEV_SETUP, &usetup);
	ioctl(ui_fd, UI_DEV_CREATE);
	sleep(1);
}


static void mouse_emit(int type, int code, int val) {
	struct input_event ie = {
		.type = type,
		.code = code,
		.value = val
	};
	ie.time.tv_sec = 0;
	ie.time.tv_usec = 0;
	
	exitif(write(ui_fd, &ie, sizeof(ie)) == -1, "Failed to whire on /dev/uinput");
}

void mouse_move_x(int dx) {
	mouse_emit(EV_REL, REL_X, dx);
	mouse_emit(EV_SYN, SYN_REPORT, 0);
}

void mouse_move_y(int dy) {
	mouse_emit(EV_REL, REL_Y, dy);
	mouse_emit(EV_SYN, SYN_REPORT, 0);
}

void mouse_clean() {
	sleep(1);
	ioctl(ui_fd, UI_DEV_DESTROY);
	close(ui_fd);
}
