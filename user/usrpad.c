#include <stdlib.h>
#include <linux/uinput.h>
#include <fcntl.h>
#include <bpf/libbpf.h>
#include <unistd.h>

#include <errno.h>
#include <signal.h>
#include <systemd/sd-daemon.h>

// Sleep time in miliseconds
// before each actions
#define SLEEP_TIME 1000
// Speed of the cursor in pixels
#define CURSOR_SPEED 2

#define PATH_SIZE 100

// path to the bpf object file
char bpf_object_path[PATH_SIZE] = "build/kerpad.bpf.o";

int running = 1;

void handler() {
	running = 0;
}

void exit_if(int condition, const char *prefix) {
	if (condition) {
		if (errno != 0) {
			perror(prefix);
		}
		else {
			fprintf( stderr, "%s\n", prefix );
		}
		exit(1);
	}
}

void block_sigint() {
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	exit_if(sigprocmask(SIG_BLOCK, &set, NULL) == -1,
			"error while blocking SIGINT");
}

void unblock_sigint() {
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	exit_if(sigprocmask(SIG_UNBLOCK, &set, NULL) == -1,
			"error while unblocking SIGINT");
}

void init_sighanlder() {
	struct sigaction old_sig;
	struct sigaction new_sig = {
		.sa_handler = handler,
		.sa_flags = 0,
	};
	exit_if(sigemptyset(&new_sig.sa_mask) == -1,
			"error on sigemptyset");
	exit_if(sigaction(SIGINT, &new_sig, &old_sig) == -1,
			"error on sigaction");
}

int init_ui_fd() {
	int fd = open("/dev/uinput", O_WRONLY|O_NONBLOCK);
	exit_if(fd == -1, "Faile to open /dev/uinput");
	struct uinput_setup usetup = {};
	
	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
	
	ioctl(fd, UI_SET_EVBIT, EV_REL);
	ioctl(fd, UI_SET_RELBIT, REL_X);
	ioctl(fd, UI_SET_RELBIT, REL_Y);
	
	usetup.id.bustype = BUS_USB;
	usetup.id.vendor = 0x1234;
	usetup.id.product = 0x5678;
	strcpy(usetup.name, "kerpad mouse");
	ioctl(fd, UI_DEV_SETUP, &usetup);
	ioctl(fd, UI_DEV_CREATE);
	sleep(1);
	
	return fd;
}

void destroy_ui_fd(int fd) {
	sleep(1);
	ioctl(fd, UI_DEV_DESTROY);
	close(fd);
}

void emit(int fd, int type, int code, int val) {
	struct input_event ie = {
		.type = type,
		.code = code,
		.value = val
	};
	ie.time.tv_sec = 0;
	ie.time.tv_usec = 0;
	
	exit_if(write(fd, &ie, sizeof(ie)) == -1, "Failed to whire on /dev/uinput");
}

void move_x(int fd, int x) {
	emit(fd, EV_REL, REL_X, x);
	emit(fd, EV_SYN, SYN_REPORT, 0);
}

void move_y(int fd, int y) {
	emit(fd, EV_REL, REL_Y, y);
	emit(fd, EV_SYN, SYN_REPORT, 0);
}

void print_help() {
	printf("Usage: ./kerpad [-b <path/to/kerpad.bpf.o>]\n");
	printf("    -b    path to the bpf object file, by default: build/kerpad.bpf.o\n");
}

int parse_args(int argc, char *argv[]) {
	int opt;
	while ((opt = getopt(argc, argv, "b:h")) != -1) {
		switch (opt) {
		case 'b':
			strcpy(bpf_object_path, optarg);
			break;
		case 'h':
			print_help();
			return 0;
		case '?':
			print_help();
			return 0;
		}
	}
	return 1;
}

int main(int argc, char *argv[]) {
	block_sigint();
	
	if (!parse_args(argc, argv))
		return EXIT_FAILURE;
	
	struct bpf_object *obj;
	struct bpf_map *map;
	int err;
	
	// Load the eBPF object file
	obj = bpf_object__open_file(bpf_object_path, NULL);
	if (libbpf_get_error(obj)) {
		fprintf(stderr, "Failed to open BPF object\n");
		return 1;
	}
	
	err = bpf_object__load(obj);
	if (err) {
		fprintf(stderr, "Failed to load BPF object\n");
		return 1;
	}
	
	struct bpf_program *prog = bpf_object__find_program_by_name(obj, "handle_touchpad");
	if (prog == NULL) {
		fprintf(stderr, "Failed to get the program\n");
		return 1;
	}
	struct bpf_link *link = bpf_program__attach_kprobe(prog, 0, "input_event");
	if (link == NULL) {
		fprintf(stderr, "Failed to attach the program\n");
		return 1;
	}
	
	// Now you can get the map file descriptor
	map = bpf_object__find_map_by_name(obj, "kerpad_map");
	if (map == NULL) {
		fprintf(stderr, "Failed to get map\n");
		return 1;
	}
	
	init_sighanlder();
	unblock_sigint();
	
	int ui_fd = init_ui_fd();
	
	sd_notify(0, "READY=1");
	while (running) {
		int key = 0;
		int value;
		// to know if x of y border was hit
		int x = 0;
		int y = 0;
		// x
		if (bpf_map__lookup_elem(map, &key, sizeof(key), &value,
								 sizeof(value), BPF_ANY) == 0
			&& value) {
			x = 1;
			if (value == -1) move_x(ui_fd, -CURSOR_SPEED);
			else if (value == 1) move_x(ui_fd, CURSOR_SPEED);
			value = 0;
			bpf_map__update_elem(map, &key, sizeof(key), &value, sizeof(value), BPF_ANY);
		}
		// y
		key = 1;
		if (bpf_map__lookup_elem(map, &key, sizeof(key), &value,
								 sizeof(value), BPF_ANY) == 0
			&& value) {
			y = 1;
			if (value == -1) move_y(ui_fd, -CURSOR_SPEED);
			else if (value == 1) move_y(ui_fd, CURSOR_SPEED);
			value = 0;
			bpf_map__update_elem(map, &key, sizeof(key), &value, sizeof(value), BPF_ANY);
		}
		if (!x || !y) usleep(SLEEP_TIME);
		else usleep(SLEEP_TIME*14);
	}
	destroy_ui_fd(ui_fd);
	
	bpf_link__detach(link);
	bpf_link__destroy(link);
	bpf_object__close(obj);
	
	return EXIT_SUCCESS;
}
