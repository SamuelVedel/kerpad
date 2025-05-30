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

// ANSI escapes
#define WHITE        "\e[00m"
#define WHITE_BOLD   "\e[00;01m"

static int running = 1;
static pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;

static int minx = -1;
static int miny = -1;

static int maxx = -1;
static int maxy = -1;

static int edge_thickness = -1;

// if non null,
// it will listen to a device
// with this name
static char *device_name = NULL;

// if non null,
// it will display coordinates
// instead of moving the mouse
static int verbose = 0;

// if non null,
// edge motion will work while
// touching the touchpad
// else it will only work while pressing
// or double touching it
static int move_touched = 0;

static struct option long_options[] = {
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

static void handler(int signum) {
	UNUSED(signum);
	pthread_mutex_lock(&running_mutex);
	running = 0;
	pthread_mutex_unlock(&running_mutex);
}

static void block_sigint() {
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	exitif(sigprocmask(SIG_BLOCK, &set, NULL) == -1,
			"error while blocking SIGINT");
}

static void unblock_sigint() {
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	exitif(sigprocmask(SIG_UNBLOCK, &set, NULL) == -1,
			"error while unblocking SIGINT");
}

static void init_sighanlder() {
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

static void *touchpad_thread(void *arg) {
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

static void *mouse_thread(void *arg) {
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

/**
 * Print an help for the option
 *
 * opt: the option to structure
 * short_opt: the char of the short version
 *            (or 0 if this option does not have a short version)
 * arg_name: the argument name, can be NULL if has_arg == no_argument
 * color: if the print option use ANSI escape or not
 * description: the option description
 */
static void print_option(struct option opt, char short_opt, char *arg_name,
						 int color, char *description) {
	char short_arg_str[255] = {};
	char long_arg_str[255] = {};
	if (opt.has_arg != no_argument) {
		sprintf(short_arg_str, " %s<%s>%s",
				opt.has_arg == optional_argument? "[": "",
				arg_name,
				opt.has_arg == optional_argument? "]": "");
		sprintf(long_arg_str, "%s=<%s>%s",
				opt.has_arg == optional_argument? "[": "",
				arg_name,
				opt.has_arg == optional_argument? "]": "");
	}
	
	printf("    ");
	if (short_opt)
		printf("%s-%c%s%s, ",
			   color? WHITE_BOLD: "", short_opt, color? WHITE: "",
			   short_arg_str);
	printf("%s--%s%s%s\n",
		   color? WHITE_BOLD: "", opt.name, color? WHITE: "",
		   long_arg_str);
	printf("%s\n", description);
}

static void print_help(int argc, char *argv[]) {
	UNUSED(argc);
	int color = isatty(STDOUT_FILENO);
	printf("Usage: %s [options]\n", argv[0]);
	print_option(long_options[0], 't', "edge_thickness", color,
				 "        Change the edge thickness");
	print_option(long_options[1], 'x', "min_x", color,
				 "        Change min x");
	print_option(long_options[2], 'X', "max_x", color,
				 "        Change max x");
	print_option(long_options[3], 'y', "min_y", color,
				 "        Change min y");
	print_option(long_options[4], 'Y', "max_y", color,
				 "        Change max y");
	print_option(long_options[5], 'n', "name", color,
				 "        Specify the touchpad name");
	print_option(long_options[6], 'a', NULL, color,
				 "        Activate edge motion even\n"
				 "        when the touchpad is justed touched");
	print_option(long_options[7], 'v', NULL, color,
				 "        Display coordinates while\n"
				 "        pressing the touchpad");
	print_option(long_options[8], 'h', NULL, color,
				 "        Display this help and exit");
}

static int parse_args(int argc, char *argv[]) {
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
