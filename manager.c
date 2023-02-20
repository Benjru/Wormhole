#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>

// TODO: Fix focus issues:
//			- When a parent is dragged and then the mouse button is released, focus should be transfered to child window
//			- When a button press is detected in a child window, it should immediately receive focus
//			- button press/release is not detected on child window - focus issue?
// TODO: Investigate shift issue (Once a window has been shifted with keybinds, cannot be dragged and vice versa)

// TODO: remove hardcoded values
int BAR_SIZE = 40;
int BORDER_WIDTH = 5;
uint32_t background_color = 0x000000;

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
	
	uint16_t win_width = g_reply_win->width;
	uint16_t win_height = g_reply_win->height;
	uint16_t root_width = g_reply_root->width;
	uint16_t root_height = g_reply_root->height;
	free(g_reply_win);
	free(g_reply_root);

	uint32_t map_values[1];
	map_values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
					XCB_EVENT_MASK_KEY_PRESS |
					XCB_EVENT_MASK_KEY_RELEASE  |
					XCB_EVENT_MASK_FOCUS_CHANGE |
					XCB_EVENT_MASK_PROPERTY_CHANGE |
					XCB_EVENT_MASK_BUTTON_PRESS |
					XCB_EVENT_MASK_BUTTON_RELEASE |
					XCB_EVENT_MASK_POINTER_MOTION;
					
	/* General geometry */
	uint32_t geometry[5];
	geometry[0] = (0.5 * root_width) - (0.5 * win_width);
	geometry[1] = (0.5 * root_height) - (0.5 * win_height);
	geometry[2] = win_width;
	geometry[3] = win_height;
	geometry[4] = BORDER_WIDTH;
	
	/* Child window geometry */
	uint32_t c_geometry[5];
	c_geometry[0] = geometry[0];
	c_geometry[1] = 0;
	c_geometry[2] = 0;
	c_geometry[3] = geometry[3];
	c_geometry[4] = geometry[4];

	/* Parent window geometry */
	uint32_t p_geometry[5];
	p_geometry[0] = geometry[0];
	p_geometry[1] = geometry[1];
	p_geometry[2] = geometry[2];
	p_geometry[3] = geometry[3] + BAR_SIZE;
	p_geometry[4] = geometry[4];
	
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
	xcb_set_input_focus(connection, XCB_INPUT_FOCUS_POINTER_ROOT, window, XCB_CURRENT_TIME);
	
	/* Create wormhole window struct */
	wormhole_window_t w_window;
	w_window.parent = parent;
	w_window.child = window;
	w_window.geometry = geometry;
	
	
	
	return;
}