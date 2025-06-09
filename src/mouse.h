#ifndef __MOUSE_H__
#define __MOUSE_H__

/**
 * Init the mouse simulation
 */
void mouse_init();

/**
 * Add dx to mouse abscissa
 * and dy to mouse ordinate
 */
void mouse_move(int dx, int dy);

/**
 * Add dx to mouse abscissa
 * coordinate
 */
void mouse_move_x(int dx);

/**
 * Add dy
 * to mouse ordinate
 * coordinate
 */
void mouse_move_y(int dy);

/**
 * Clean the mouse simulation
 */
void mouse_clean();

#endif // !__MOUSE_H__
