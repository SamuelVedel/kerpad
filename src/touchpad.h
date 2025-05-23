#ifndef __TOUCHPAD_H__
#define __TOUCHPAD_H__

struct touchpad_info {
	int x;
	int y;
	// -1: touching the left edge
	//  1: touching the right edge
	int edgex;
	// -1: touching the top edge
	//  1: touching the bottom edge
	int edgey;
	int touching;
	int pressing;
	int double_touching;
};
typedef struct touchpad_info touchpad_info_t;

struct touchpad_settings {
	// if non null, it will search
	// for a touchpad with this name
	char *device_name;
	// the below variables describe the limits
	// on the touchpad after which it is concidere
	// beeing the edge
	//
	// if one of those value is negative,
	// then it will be determined automatically
	// with the touchpad dimentions and the
	// edge_thickness variable
	int minx;
	int maxx;
	int miny;
	int maxy;
	// this value will be used to determine
	// non defined edge limits
	//
	// if this value is negative, then a
	// default value will be used
	int edge_thickness;
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
