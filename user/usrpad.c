#include <stdlib.h>
#include <X11/Xlib.h>
#include <bpf/libbpf.h>
#include <unistd.h>

#include <errno.h>
#include <signal.h>

#define CURSOR_SPEED 2

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

int main() {
	struct bpf_object *obj;
	struct bpf_map *map;
	int err;
	
	// Load the eBPF object file
	obj = bpf_object__open_file("build/kerpad.bpf.o", NULL);
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
	
	while (running) {
		Display *dpy = XOpenDisplay(NULL);
		int key = 0;
		int value;
		// x
		if (bpf_map__lookup_elem(map, &key, sizeof(key), &value, sizeof(value), BPF_ANY) == 0) {
			if (value == -1) XWarpPointer(dpy, None, None, 0, 0, 0, 0, -CURSOR_SPEED, 0);
			else if (value == 1) XWarpPointer(dpy, None, None, 0, 0, 0, 0, CURSOR_SPEED, 0);
			value = 0;
			bpf_map__update_elem(map, &key, sizeof(key), &value, sizeof(value), BPF_ANY);
		}
		// y
		key = 1;
		if (bpf_map__lookup_elem(map, &key, sizeof(key), &value, sizeof(value), BPF_ANY) == 0) {
			if (value == -1) XWarpPointer(dpy, None, None, 0, 0, 0, 0, 0, -CURSOR_SPEED);
			else if (value == 1) XWarpPointer(dpy, None, None, 0, 0, 0, 0, 0, CURSOR_SPEED);
			value = 0;
			bpf_map__update_elem(map, &key, sizeof(key), &value, sizeof(value), BPF_ANY);
		}
		XCloseDisplay(dpy);
		usleep(1000);
	}
	
	bpf_link__detach(link);
	bpf_link__destroy(link);
	bpf_object__close(obj);
	
	return EXIT_SUCCESS;
}
