#ifndef MANAGER_H
#define MANAGER_H

#include <xcb/xcb.h>

extern int BAR_SIZE;
extern int BORDER_WIDTH;
extern uint32_t background_color;

/* Struct used for tracking window properties */
typedef struct 
{	
	xcb_window_t parent;
	xcb_window_t child;
	int geometry[5];
} wormhole_window_t;

/* Struct for tracking information about all windows */
typedef struct // TODO: implement relavent functionality
{
	wormhole_window_t *windows;
	wormhole_window_t focused;
	int num_windows;
} manager_data_t; // can this be deleted?

void wormhole_destroy();
xcb_window_t wormhole_get_parent();
xcb_window_t wormhole_get_child();
void wormhole_create_window();

#endif