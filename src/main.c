#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include "touchpad.h"
#include "mouse.h"
#include "util.h"

#define UNUSED(x) ((void)x);

#define SLEEP_TIME 3000
//#define CORNER_SLEEP_TIME (SLEEP_TIME*1414/1000)
#define CORNER_SLEEP_TIME (SLEEP_TIME*2)
#define CURSOR_SPEED 1

int running = 1;
pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;

int minx = -1;
int miny = -1;

int maxx = -1;
int maxy = -1;

int edge_thickness = -1;

// if non null,
// it will listen to a device
// with this name
char *device_name = NULL;

// if non null,
// it will display coordinates
// instead of moving the mouse
int verbose = 0;

// if non null,
// edge motion will work while
// touching the touchpad
// else it will only work while pressing
// or double touching it
int move_touched = 0;

void handler(int signum) {
	UNUSED(signum);
	pthread_mutex_lock(&running_mutex);
	running = 0;
	pthread_mutex_unlock(&running_mutex);
}

void block_sigint() {
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	exitif(sigprocmask(SIG_BLOCK, &set, NULL) == -1,
			"error while blocking SIGINT");
}

void unblock_sigint() {
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	exitif(sigprocmask(SIG_UNBLOCK, &set, NULL) == -1,
			"error while unblocking SIGINT");
}

void init_sighanlder() {
	struct sigaction old_sig;
	struct sigaction new_sig = {
		.sa_handler = handler,
		.sa_flags = 0,
	};
	exitif(sigemptyset(&new_sig.sa_mask) == -1,
			"error on sigemptyset");
	exitif(sigaction(SIGINT, &new_sig, &old_sig) == -1,
			"error on sigaction");
}

void *touchpad_thread(void *arg) {
	UNUSED(arg);
	
	pthread_mutex_lock(&running_mutex);
	while (running) {
		pthread_mutex_unlock(&running_mutex);
		//printf("touch\n");
		touchpad_read_next_event();
		pthread_mutex_lock(&running_mutex);
	}
	pthread_mutex_unlock(&running_mutex);
	touchpad_signal();
	
	return NULL;
}

void *mouse_thread(void *arg) {
	UNUSED(arg);
	
	pthread_mutex_lock(&running_mutex);
	while (running) {
		pthread_mutex_unlock(&running_mutex);
		touchpad_info_t info = {};
		touchpad_get_info(&info);
		
		int active = (info.pressing || info.double_touching)
			|| (move_touched && info.touching);
		if (active) {
			if (verbose) printf("x:%d y:%d\n", info.x, info.y);
			if (info.edgex) mouse_move_x(info.edgex*CURSOR_SPEED);
			if (info.edgey) mouse_move_y(info.edgey*CURSOR_SPEED);
		}
		
		if (!active) touchpad_wait();
		else if (!info.edgex || !info.edgey) usleep(SLEEP_TIME);
		else usleep(CORNER_SLEEP_TIME);
		
		pthread_mutex_lock(&running_mutex);
	}
	pthread_mutex_unlock(&running_mutex);
	
	return NULL;
}

void print_help(int argc, char *argv[]) {
	UNUSED(argc);
	printf("Usage: %s [options]\n", argv[0]);
	printf("    -t <edge_thickness>, --thickness=<edge_thickness>\n");
	printf("        Change the edge thickness\n");
	printf("    -x <min_x>, --minx=<min_x>\n");
	printf("        Change min x\n");
	printf("    -X <max_x>, --maxx=<max_x>\n");
	printf("        Change max x\n");
	printf("    -y <min_y>, --miny=<min_y>\n");
	printf("        Change min y\n");
	printf("    -Y <max_y>, --maxy=<max_y>\n");
	printf("        Change max y\n");
	printf("    -n <name>, --name=<name>\n");
	printf("        Specify the touchpad name\n");
	printf("    -a, --always\n");
	printf("        Activate edge motion even\n");
	printf("        when the touchpad is justed touched\n");
	printf("    -v, --verbose\n");
	printf("        Display coordinates while\n");
	printf("        pressing the touchpad\n");
	printf("    -h, --help\n");
	printf("        Display this help and exit\n");
}

int parse_args(int argc, char *argv[]) {
	struct option long_options[] = {
		{"thickness", required_argument, NULL, 't'},
		
		{"minx", required_argument, NULL, 'x'},
		{"maxx", required_argument, NULL, 'X'},
		
		{"miny", required_argument, NULL, 'y'},
		{"maxy", required_argument, NULL, 'Y'},
		
		{"name", required_argument, NULL, 'n'},
		{"always", no_argument, NULL, 'a'},
		{"verbose", no_argument, NULL, 'v'},
		{"help", no_argument, NULL, 'h'},
		
		{"hey", optional_argument, NULL, '\n'},
		{0, 0, 0, 0},
	};
	while (1) {
		int opt = getopt_long(argc, argv, "t:x:X:y:Y:n:avh", long_options, NULL);
		if (opt == -1) break;
		
		switch (opt) {
		case 't':
			edge_thickness = atoi(optarg);
			break;
		case 'x':
			minx = atoi(optarg);
			break;
		case 'X':
			maxx = atoi(optarg);
			break;
		case 'y':
			miny = atoi(optarg);
			break;
		case 'Y':
			maxy = atoi(optarg);
			break;
		case 'n':
			device_name = optarg;
			break;
		case 'a':
			move_touched = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'h':
			print_help(argc, argv);
			return 0;
		case '\n':
			printf(" o/ <(hey");
			if (optarg) printf(" %s)\n", optarg);
			else printf(")\n");
			printf("/|\n");
			printf("/ \\\n");
			break;
		case '?':
			print_help(argc, argv);
			return 0;
		}
	}
	return 1;
}

int main(int argc, char *argv[]) {
	block_sigint();
	if (!parse_args(argc, argv))
		return EXIT_FAILURE;
	
	touchpad_settings_t settings = {
		.device_name = device_name,
		.minx = minx,
		.maxx = maxx,
		.miny = miny,
		.maxy = maxy,
		.edge_thickness = edge_thickness
	};
	if (touchpad_init(&settings) < 0) {
		return EXIT_FAILURE;
	}
	mouse_init();
	
	init_sighanlder();
	unblock_sigint();
	
	pthread_t touchap_th;
	pthread_t mouse_th;
	
	pthread_create(&touchap_th, NULL, touchpad_thread, NULL);
	pthread_create(&mouse_th, NULL, mouse_thread, NULL);
	
	pthread_join(touchap_th, NULL);
	pthread_join(mouse_th, NULL);
	
	touchpad_clean();
	mouse_clean();
	
	return EXIT_SUCCESS;
}
