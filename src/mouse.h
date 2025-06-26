#ifndef __MOUSE_H__
#define __MOUSE_H__

typedef struct mouse mouse_t;

/**
 * Init the mouse simulation
 *
 * name: the name of the simulated mouse
 */
mouse_t *mouse_init(const char *name);

/**
 * Add dx to mouse abscissa
 * and dy to mouse ordinate
 */
void mouse_move(mouse_t *mouse, int dx, int dy);

/**
 * Add dx to mouse abscissa
 * coordinate
 */
void mouse_move_x(mouse_t *mouse, int dx);

/**
 * Add dy
 * to mouse ordinate
 * coordinate
 */
void mouse_move_y(mouse_t *mouse, int dy);

/**
 * Clean the mouse simulation
 */
void mouse_clean(mouse_t *mouse);

#endif // !__MOUSE_H__
