#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>

#include "touchpad.h"
#include "mouse.h"
#include "util.h"

#define UNUSED(x) ((void)x);

#define DEFAULT_SLEEP_TIME 3000
#define CORNER_SLEEP_TIME(time) (time*1414/1000)
#define CURSOR_SPEED 1

// ANSI escapes
#define WHITE        "\e[00m"
#define WHITE_BOLD   "\e[00;01m"

#define TO_STR(x) #x
#define MACRO_TO_STR(x) TO_STR(x)

#define EXPLANATION_AREA_LEN 50
#define OPT_DESCRIPTION_AREA_OFFSET 8
#define OPT_DESCRIPTION_AREA_LEN EXPLANATION_AREA_LEN-OPT_DESCRIPTION_AREA_OFFSET

#define NO_EDGE_PROTECTION_OPTION 1
#define DISABLE_DOUBLE_TAP_OPTION 2

static bool running = true;
static pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;

static int minx = -1;
static int miny = -1;

static int maxx = -1;
static int maxy = -1;

static int edge_thickness = -1;

static int sleep_time = DEFAULT_SLEEP_TIME;

static bool no_edge_protection = false;
static bool disable_double_tap = false;

static int list = LIST_NO;

// if non null,
// it will listen to a device
// with this name
static char *device_name = NULL;

// if non null,
// it will display coordinates
static bool verbose = false;

// if non null,
// edge motion will work while
// touching the touchpad
// else it will only work while pressing
// or double tapping it
static bool move_touched = false;

static struct option long_options[] = {
	{"thickness", required_argument, NULL, 't'},
	
	{"minx", required_argument, NULL, 'x'},
	{"maxx", required_argument, NULL, 'X'},
	
	{"miny", required_argument, NULL, 'y'},
	{"maxy", required_argument, NULL, 'Y'},
	
	{"sleep_time", required_argument, NULL, 's'},
	
	{"name", required_argument, NULL, 'n'},
	{"always", no_argument, NULL, 'a'},
	{"no_edge_protection", no_argument, NULL, NO_EDGE_PROTECTION_OPTION},
	{"disable_double_tap", no_argument, NULL, DISABLE_DOUBLE_TAP_OPTION},
	{"list", optional_argument, NULL, 'l'},
	{"verbose", no_argument, NULL, 'v'},
	{"help", no_argument, NULL, 'h'},
	
	{"hey", optional_argument, NULL, '\n'},
	{0, 0, 0, 0},
};

static void handler(int signum) {
	UNUSED(signum);
	pthread_mutex_lock(&running_mutex);
	running = false;
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
		
		bool active = info.pressed
			|| (!disable_double_tap && info.double_tapped)
			|| (move_touched && info.touched);
		if (active) {
			if (verbose) printf("x:%d y:%d\n", info.x, info.y);
			
			if (info.edgex && info.edgey) {
				mouse_move(info.edgex*CURSOR_SPEED, info.edgey*CURSOR_SPEED);
			} else if (info.edgex) {
				mouse_move_x(info.edgex*CURSOR_SPEED);
			} else if (info.edgey) {
				mouse_move_y(info.edgey*CURSOR_SPEED);
			}
		}
		
		if (!active) touchpad_wait();
		else if (!info.edgex || !info.edgey) usleep(sleep_time);
		else usleep(CORNER_SLEEP_TIME(sleep_time));
		
		pthread_mutex_lock(&running_mutex);
	}
	pthread_mutex_unlock(&running_mutex);
	
	return NULL;
}

/**
 * Return the size of the first word in the str
 */
static int word_len(const char *str) {
	int len = 0;
	while (*str != ' ' && *str != 0 && *str != '\n') {
		++len;
		++str;
	}
	return len;
}

/**
 * Print the given text in an area starting at offset,
 * and of length len
 */
static void print_text_area(int offset, int len, const char *text) {
	for (int i = 0; i < offset; ++i) printf(" ");
	int line_len = 0;
	while (*text != 0) {
		int wlen = word_len(text);
		if ((line_len+wlen > len && line_len > 0) || *text == '\n') {
			printf("\n");
			if (*text == '\n') {
				++text;
				while (*text == ' ') ++text;
			}
			for (int i = 0; i < offset; ++i) printf(" ");
			line_len = 0;
			continue;
		}
		fwrite(text, 1, wlen, stdout);
		printf(" ");
		text += wlen;
		line_len += wlen;
		while (*text == ' ') ++text;
	}
	printf("\n");
}

/**
 * Print an help for the option
 *
 * opt: the option to structure
 * short_opt: the char of the short version
 *            (or 0 if this option does not have a short version)
 * arg_name: the argument name, can be NULL if opt->has_arg == no_argument
 * color: if the print option use ANSI escape or not
 * description: the option description
 */
static void print_option(const struct option *opt, char short_opt, const char *arg_name,
						 bool color, const char *description) {
	char short_arg_str[255] = {};
	char long_arg_str[255] = {};
	if (opt->has_arg != no_argument) {
		if (opt->has_arg == required_argument)
			sprintf(short_arg_str, " %s", arg_name);
		sprintf(long_arg_str, "%s=%s%s",
				opt->has_arg == optional_argument? "[": "",
				arg_name,
				opt->has_arg == optional_argument? "]": "");
	}
	
	printf("    ");
	if (short_opt)
		printf("%s-%c%s%s, ",
			   color? WHITE_BOLD: "", short_opt, color? WHITE: "",
			   short_arg_str);
	printf("%s--%s%s%s\n",
		   color? WHITE_BOLD: "", opt->name, color? WHITE: "",
		   long_arg_str);
	print_text_area(OPT_DESCRIPTION_AREA_OFFSET,
					OPT_DESCRIPTION_AREA_LEN, description);
}

static void print_help(int argc, char *argv[]) {
	UNUSED(argc);
	bool color = isatty(STDOUT_FILENO);
	int i = 0;
	printf("Usage: %s [options]\n", argv[0]);
	print_option(long_options+i++, 't', "edge_thickness", color,
				 "Change the edge thickness");
	print_option(long_options+i++, 'x', "min_x", color,
				 "Change min_x");
	print_option(long_options+i++, 'X', "max_x", color,
				 "Change max_x");
	print_option(long_options+i++, 'y', "min_y", color,
				 "Change min_y");
	print_option(long_options+i++, 'Y', "max_y", color,
				 "Change max_y");
	print_option(long_options+i++, 's', "sleep_time", color,
				 "When edge motion is triggered, the mouse will move "
				 "one pixel each sleep_time microseconds. The sleep time "
				 "will be slightly longer when touching a corner. "
				 "The default sleep time is "MACRO_TO_STR(DEFAULT_SLEEP_TIME));
	print_option(long_options+i++, 'n', "name", color,
				 "Specify the touchpad name");
	print_option(long_options+i++, 'a', NULL, color,
				 "Activate edge motion even "
				 "when the touchpad is just touched");
	print_option(long_options+i++, 0, NULL, color,
				 "Don't ignore touches made beyond the edge limits");
	print_option(long_options+i++, 0, NULL, color,
				 "Don't concider the touchpad pressed, when it is double tapped");
	print_option(long_options+i++, 'l', "WHICH", color,
				 "List caracteritics of input devices. WHICH value can be:\n"
				 "- candidates: list only candidate devices (default value)\n"
				 "- all: list all input devices");
	print_option(long_options+i++, 'v', NULL, color,
				 "Display coordinates while "
				 "pressing the touchpad\n"
				 "If combine with -a, it will display the coordinates "
				 "even when the touchpad is just touched");
	print_option(long_options+i++, 'h', NULL, color,
				 "Display this help and exit");
	
	printf("\n");
	print_text_area(0, EXPLANATION_AREA_LEN,
					"This program implements a customizable edge motion "
					"to make your mouse move automatically while touching "
					"the edge of your touchpad.");
	printf("\n");
	printf("    ┌───────────────────────────────────┐\n");
	printf("    │   min_x     thickness     max_x   │\n");
	printf("    │min_y──────────────────────────┐   │\n");
	printf("    │   │                           │   │\n");
	printf("    │   │                           │   │\n");
	printf("    │   │                           │   │\n");
	printf("    │   │         touchpad          │   │\n");
	printf("    │   │                           │   │\n");
	printf("    │   │                           │   │\n");
	printf("    │   │                           │   │\n");
	printf("    │max_y──────────────────────────┘   │\n");
	printf("    │                                   │\n");
	printf("    └───────────────────────────────────┘\n");
	
	print_text_area(0, EXPLANATION_AREA_LEN,
					"The above drawing represents you touchpad. "
					"When your touchpad is pressed (or double tapped), "
					"if your finger is between the two squares (at "
					"the edge of the touchpad), then this program "
					"will make the mouse "
					"move automatically. By default, the edge limits "
					"are defined automatically in relation to your "
					"touchpad size and a default thickness (="
					MACRO_TO_STR(DEFAULT_EDGE_THICKNESS)"). "
					"If you don't like the default limits, you can use "
					"options to change the thickness or directly the limit "
					"values.");
}

/**
 * Parse and handle command line arguments
 *
 * return 0 on success
 *        1 if the help was printed
 *        a negative value on error
 */
static int parse_args(int argc, char *argv[]) {
	while (1) {
		int opt = getopt_long(argc, argv, "t:x:X:y:Y:s:n:lavh", long_options, NULL);
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
		case 's':
			sleep_time = atoi(optarg);
			break;
		case 'n':
			device_name = optarg;
			break;
		case 'a':
			move_touched = 1;
			break;
		case NO_EDGE_PROTECTION_OPTION:
			no_edge_protection = true;
			break;
		case DISABLE_DOUBLE_TAP_OPTION:
			disable_double_tap = true;
			break;
		case 'l':
			if (!optarg || !strcmp(optarg, "candidates")) {
				list = LIST_CANDIDATES;
			} else if (!strcmp(optarg, "all")) {
				list = LIST_ALL;
			} else {
				print_help(argc, argv);
				return -1;
			}
			break;
		case 'v':
			verbose = 1;
			break;
		case 'h':
			print_help(argc, argv);
			return 1;
		case '\n':
			printf(" o/ <(hey");
			if (optarg) printf(" %s)\n", optarg);
			else printf(")\n");
			printf("/|\n");
			printf("/ \\\n");
			break;
		case '?':
			print_help(argc, argv);
			return -1;
		}
	}
	return 0;
}

int main(int argc, char *argv[]) {
	block_sigint();
	int parse_result = parse_args(argc, argv);
	if (parse_result < 0)
		return EXIT_FAILURE;
	else if (parse_result == 1)
		return EXIT_SUCCESS;
	
	touchpad_settings_t settings = {
		.device_name = device_name,
		.minx = minx,
		.maxx = maxx,
		.miny = miny,
		.maxy = maxy,
		.edge_thickness = edge_thickness,
		.no_edge_protection = no_edge_protection,
		.list = list,
	};
	if (touchpad_init(&settings) < 0) {
		return EXIT_FAILURE;
	}
	if (list != LIST_NO) {
		touchpad_clean();
		return EXIT_SUCCESS;
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
