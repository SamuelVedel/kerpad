#ifndef __TOUCHPAD_H__
#define __TOUCHPAD_H__

struct touchpad_info {
	int x;
	int y;
	int touching;
	int pressing;
	int double_touching;
};
typedef struct touchpad_info touchpad_info_t;

struct touchpad_settings {
	// if non null, it will search
	// for a touchpad with this name
	char *device_name;
	int minx;
	int maxx;
	int miny;
	int maxy;
};
typedef struct touchpad_settings touchpad_settings_t;

/**
 * Init the needed things for touchpad
 * event polling
 *
 * Return a negative value if no touchpad
 * is found
 */
int touchpad_init(touchpad_settings_t *settings);

/**
 * Clean the touchpad event polling
 */
void touchpad_clean();

/**
 * Read the next touchpad event
 */
void touchpad_read_next_event();

/**
 * Write informations about
 * the touchpad
 */
void touchpad_get_info(touchpad_info_t *info);

/**
 * Wait for the touchpad to be touched or pressed
 */
void touchpad_wait();

/**
 * Signal a thread waiting for the touchap
 * to be touched or pressed
 */
void touchpad_signal();

#endif // !__TOUCHPAD_H__
