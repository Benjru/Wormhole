#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <string.h>
#include "manager.h"

// TODO: make getBorderWidth function, replace duplicate in event_handler
// TODO: remove hardcoded values
int BAR_SIZE = 40;
int BORDER_WIDTH = 5;
uint32_t background_color = 0x000000;
int num_active_windows = 0;
wormhole_window_t *active_windows;
// manager_data_t??

/* Method for destroying a window */
void wormhole_destroy(xcb_window_t window, xcb_connection_t *connection)
{
    int i;
    for (i = 0; i < num_active_windows; i++)
    {
        if (active_windows[i].parent == window || active_windows[i].child == window)
        {
            xcb_destroy_window(connection, active_windows[i].child);
            xcb_destroy_window(connection, active_windows[i].parent);

			int j;
            for (j = i; j < num_active_windows-1; j++)
            {
                active_windows[j] = active_windows[j+1];
            }
            num_active_windows--;
            active_windows = realloc(active_windows, sizeof(wormhole_window_t) * num_active_windows);
            i--;
        }
    }
    xcb_flush(connection);
}

/* Method for fetching parent window */
xcb_window_t wormhole_get_parent(xcb_window_t window, xcb_window_t root)
{
	int i;
	for(i=0; i<num_active_windows; i++)
	{
		if(active_windows[i].parent == window)
		return active_windows[i].parent; // used to be parent?
	
		if(active_windows[i].child == window)
		return active_windows[i].parent;
	}
	return root;
}

/* Method for fetching child window */
xcb_window_t wormhole_get_child(xcb_window_t window, xcb_window_t root)
{
	int i;
	for(i=0; i<num_active_windows; i++)
	{
		if(active_windows[i].parent == window)
		return active_windows[i].child; // used to be parent?
	
		if(active_windows[i].child == window)
		return active_windows[i].child;
	}
	return root;
}

/* Create a new window */
void wormhole_create_window(xcb_window_t window, xcb_screen_t *screen, xcb_connection_t *connection)
{
	/* Retreive requested geometry */
	xcb_get_geometry_reply_t *g_reply_win = xcb_get_geometry_reply(connection, xcb_get_geometry(connection, window), NULL);
	if(!g_reply_win)
	{
		printf("[Wormhole] Error - Failed to retreive geometry of window requesting mapping.\n");
		return;
	}
	
	xcb_window_t root_window = screen->root;
	xcb_get_geometry_reply_t *g_reply_root = xcb_get_geometry_reply(connection, xcb_get_geometry(connection, root_window), NULL);
	if(!g_reply_root)
	{
		printf("[Wormhole] Error - Failed to retreive geometry of root window.\n");
		return;
	}
	
	/* Extract specified geometry */
	uint16_t win_width = g_reply_win->width;
	uint16_t win_height = g_reply_win->height;
	uint16_t root_width = g_reply_root->width;
	uint16_t root_height = g_reply_root->height;
	free(g_reply_win);
	free(g_reply_root);
	
	/* Main geometry */
	uint32_t geometry[5];
	geometry[0] = (0.5 * root_width) - (0.5 * win_width);
	geometry[1] = (0.5 * root_height) - (0.5 * win_height);
	geometry[2] = win_width;
	geometry[3] = win_height;
	geometry[4] = BORDER_WIDTH;
	
	/* Handle bad / unspecified geometry */
	if(geometry[2] < 5 || geometry[3] < 5)
	{
		geometry[2] = (0.66 * root_width);
		geometry[3] = (0.66 * root_height);
		geometry[0] = (0.5 * root_width) - (0.5 * geometry[2]);
		geometry[1] = (0.5 * root_height) - (0.5 * geometry[3]);
	}
	
	/* Child window geometry */
	uint32_t c_geometry[5];
	c_geometry[0] = 0;
	c_geometry[1] = BAR_SIZE;
	c_geometry[2] = geometry[2];
	c_geometry[3] = geometry[3];
	c_geometry[4] = 0;
	
	/* Parent window geometry */
	uint32_t p_geometry[5];
	p_geometry[0] = geometry[0];
	p_geometry[1] = geometry[1];
	p_geometry[2] = geometry[2];
	p_geometry[3] = geometry[3] + BAR_SIZE;
	p_geometry[4] = geometry[4];
	
	/* Event masks */
	uint32_t map_values[1];
	map_values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
					XCB_EVENT_MASK_KEY_PRESS |
					XCB_EVENT_MASK_KEY_RELEASE  |
					XCB_EVENT_MASK_FOCUS_CHANGE |
					XCB_EVENT_MASK_PROPERTY_CHANGE |
					XCB_EVENT_MASK_BUTTON_PRESS |
					XCB_EVENT_MASK_BUTTON_RELEASE |
					// XCB_EVENT_MASK_ENTER_WINDOW | 
					XCB_EVENT_MASK_LEAVE_WINDOW |
					XCB_EVENT_MASK_POINTER_MOTION;
	
	/* Reparent newly generated window */
	xcb_window_t parent = xcb_generate_id(connection);
	xcb_create_window(connection,
					  XCB_COPY_FROM_PARENT, 
					  parent,
					  root_window,
					  p_geometry[0],
					  p_geometry[1], 
					  p_geometry[2], 
					  p_geometry[3], 
					  p_geometry[4], 
					  XCB_WINDOW_CLASS_INPUT_OUTPUT,
					  screen->root_visual,
					  XCB_CW_BACK_PIXEL,
					  (uint32_t[]) {background_color});
	xcb_reparent_window(connection, window, parent, 0, BAR_SIZE);
	xcb_map_window(connection, parent);
	xcb_map_window(connection, window);	
	
	/* Configure child window */
	xcb_configure_window(connection, window, XCB_CONFIG_WINDOW_X |
										  XCB_CONFIG_WINDOW_Y |
										  XCB_CONFIG_WINDOW_WIDTH |
										  XCB_CONFIG_WINDOW_HEIGHT | 
										  XCB_CONFIG_WINDOW_BORDER_WIDTH, c_geometry);									  
	xcb_change_window_attributes(connection, parent, XCB_CW_EVENT_MASK, map_values);										  
	xcb_change_window_attributes(connection, window, XCB_CW_EVENT_MASK, map_values);
	xcb_set_input_focus(connection, XCB_INPUT_FOCUS_PARENT, window, XCB_CURRENT_TIME);
	
	/* Create wormhole window struct */
	wormhole_window_t w_window;
	w_window.parent = parent;
	w_window.child = window;
	memcpy(w_window.geometry, geometry, sizeof(geometry));
	if(num_active_windows == 0)
		active_windows = (wormhole_window_t *) malloc(sizeof(wormhole_window_t));
	else
		active_windows = (wormhole_window_t *) realloc(active_windows, sizeof(wormhole_window_t) * num_active_windows);
	active_windows[num_active_windows] = w_window;
	num_active_windows++;
	
	// TODO: Complete window tracking logic (necessary for more advanced functionality)
	
	return;
}