#ifndef __TOUCHPAD_H__
#define __TOUCHPAD_H__

#include <stdbool.h>

#define DEFAULT_EDGE_THICKNESS 250

#define LIST_NO         0
#define LIST_CANDIDATES 1
#define LIST_ALL        2

typedef struct touchpad touchpad_t;

struct touchpad_info {
	// Coordinates on the touchpad
	int x;
	int y;
	// -1: touching the left edge
	//  1: touching the right edge
	int edgex;
	// -1: touching the top edge
	//  1: touching the bottom edge
	int edgey;
	// true if the touchpad is touched
	// Touches made beyond the edge limits are ingored
	// unless no_edge_protection is true
	bool touched;
	// true if the touchpad is touched beyond
	// the edge limits
	bool edge_touched;
	// true if the touchpad is pressed
	// Presses made beyond the edge limits are ingored
	// unless no_edge_protection is true
	bool pressed;
	// true if the touchpad is double tapped
	// Double taps made beyond the edge limits are ingored
	// unless no_edge_protection is true
	bool double_tapped;
};
typedef struct touchpad_info touchpad_info_t;

struct touchpad_settings {
	// if non null, it will search
	// for a touchpad with this name
	char *device_name;
	
	// the below variables describe the limits
	// on the touchpad after which it is considered
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
	
	// if false, it will ignore touches made beyond
	// the edge limites
	bool no_edge_protection;
	
	// if its value is LIST_CANDIDATES
	// touchpad_init will list the caracteristics
	// of candidate devices
	// if its value is LIST_ALL it will list the
	// caracteristics of all input devices
	int list;
};
typedef struct touchpad_settings touchpad_settings_t;

/**
 * Init the needed things for touchpad
 * event polling
 *
 * Return a NULL if no touchpad
 * is found
 */
touchpad_t *touchpad_init(touchpad_settings_t *settings);

/**
 * Clean the touchpad event polling
 */
void touchpad_clean(touchpad_t *touchpad);

/**
 * Read the next touchpad event
 */
void touchpad_read_next_event(touchpad_t *touchpad);

/**
 * Write informations about
 * the touchpad
 */
void touchpad_get_info(touchpad_t *touchpad, touchpad_info_t *info);

/**
 * Wait for the touchpad to be touched
 *
 * Touches made beyond the edge limits are ingored
 * unless no_edge_protection is true
 */
void touchpad_wait_touch(touchpad_t *touchpad);

/**
 * Restart all the threads that are waiting for the touchap
 * to be touched
 */
void touchpad_broadcast_touch(touchpad_t *touchpad);

/**
 * Wait for the touchpad to be pressed or double tapped
 *
 * Pressed and double taps made beyon the edge limits are ingored
 * unless no_edge_protection is true
 */
void touchpad_wait_press(touchpad_t *touchpad);

/**
 * Restart all the threads that are waiting for the touchap
 * to be pressed or double tapped
 */
void touchpad_broadcast_press(touchpad_t *touchpad);

/**
 * Wait for the touchpad to be touched, pressed or double tapped
 *
 * Touches, presses and double taps made beyond the edge limits are ingored
 * unless no_edge_protection is true
 */
void touchpad_wait_touch_or_press(touchpad_t *touchpad);

/**
 * Wait for the touchpad to be touched beyond the edge limits
 */
void touchpad_wait_edge_touch(touchpad_t *touchpad);

/**
 * Restart all the threads that are  waiting for the touchap
 * to be touched beyond the edge limits
 */
void touchpad_broadcast_edge_touch(touchpad_t *touchpad);

#endif // !__TOUCHPAD_H__
