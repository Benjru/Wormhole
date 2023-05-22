#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_keysyms.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <inttypes.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-x11.h>
#include "action_processor.h"
#include "keybinds.h"
#include "manager.h"

#define DEFAULT_CURSOR_GLYPH 68

static int MAX_COMBO_LENGTH = 5;
static uint8_t *BASE_EVENT_OUT;

/* Initialization and event handling */
int main(int argc, char *argv)
{	
	/* Connection initialization */
	xcb_connection_t *connection = xcb_connect(NULL, NULL);
	if(xcb_connection_has_error(connection))
		
	{
		printf("[Wormhole] Failed to start Wormhole, is another window manager already running?\n");
		return 0;
	}

	/* Setup for screen and root window */
	const xcb_setup_t *setup = xcb_get_setup(connection);
	xcb_screen_iterator_t iterator = xcb_setup_roots_iterator(setup);
	xcb_screen_t *screen = iterator.data;
	xcb_window_t root_window = screen->root;
	
	/* WM property setup */
	char *wm_name = "Wormhole";
	xcb_change_property(connection,
						XCB_PROP_MODE_REPLACE,
						root_window,
						XCB_ATOM_WM_NAME,
						XCB_ATOM_STRING,
						8,
						strlen(wm_name),
						wm_name);

	/* Setup for device input, event masks, and substructure redirection */
	uint32_t mask;
	uint32_t valwin[1];
	uint32_t device_id;
	int xkb_extension;
	
	xcb_key_symbols_t *key_symbols;
	key_symbols = xcb_key_symbols_alloc(connection);
				
	mask = XCB_CW_EVENT_MASK;
	valwin[0] = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | 
				XCB_EVENT_MASK_KEY_PRESS |
				XCB_EVENT_MASK_KEY_RELEASE |
				XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
				XCB_EVENT_MASK_FOCUS_CHANGE |
				XCB_EVENT_MASK_PROPERTY_CHANGE;	
		
	xcb_change_window_attributes_checked(connection, root_window, mask, valwin);
	xcb_flush(connection);
	
	xkb_extension = xkb_x11_setup_xkb_extension(connection,
												XKB_X11_MIN_MAJOR_XKB_VERSION,
												XKB_X11_MIN_MINOR_XKB_VERSION,
												XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, 
												NULL, 
												NULL, 
												BASE_EVENT_OUT, 
												NULL);
	/* Keymap and keyboard state setup */
	if(!xkb_extension)
	{
		printf("[Wormhole] Failed to setup xkb extension - is xkbcommon up to date?");
		xcb_disconnect(connection);
		return 1;
	}

	struct xkb_context *ctx;
	ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if(!ctx)
	{
		printf("[Wormhole] Failed to obtain keyboard context, terminating process.\n");
		xcb_disconnect(connection);
		return 1;
	}

	device_id = xkb_x11_get_core_keyboard_device_id(connection);
	if(device_id == -1)
	{
		printf("[Wormhole] Failed to obtain keyboard device identifier, terminating process.\n");
		xcb_disconnect(connection);
		return 1;
	}

	struct xkb_keymap *keymap;
	keymap = xkb_x11_keymap_new_from_device(ctx, connection, device_id, XKB_KEYMAP_COMPILE_NO_FLAGS);
	if(!keymap)
	{
		printf("[Wormhole] Failed to obtain keymap, terminating process.\n");
		xcb_disconnect(connection);
		return 1;
	}

	struct xkb_state *state;
	state = xkb_x11_state_new_from_device(keymap, connection, device_id);
	if(!state)
	{
		printf("[Wormhole] Failed to obtain keyboard state, terminating process.\n");
		xcb_disconnect(connection);
		return 1;
	}
	printf("[Wormhole] Launch successful.\n");
	 
	/* Load keybind configuration */
	int combo_index = 0;
	keybinds_t *keybinds;
	keybinds = read_keybind_configuration();
	xkb_keysym_t combo_keysyms[MAX_COMBO_LENGTH];
	printf("[Wormhole] Successfully loaded keybind configurations.\n");

	/* Mouse attributes */
	int16_t mouse_x = 0;
	int16_t mouse_y = 0;
	int16_t start_mouse_x = 0;
	int16_t start_mouse_y = 0;
	
	bool button_held = false;
	bool left_hovering = false;
    bool right_hovering = false;
    bool top_hovering = false;
    bool bottom_hovering = false;
	
	bool left_dragging = false;
	bool right_dragging = false;
	bool top_dragging = false;
	bool bottom_dragging = false;
	bool top_left_dragging = false;
	bool top_right_dragging = false;
	bool bottom_right_dragging = false;
	bool bottom_left_dragging = false;
	
	/* Cursor glyph setup (default & edges) */
	xcb_font_t cursorFont = xcb_generate_id(connection);
	xcb_void_cookie_t fontCookie = xcb_open_font(connection, cursorFont, strlen("cursor"), "cursor");
	
	xcb_cursor_t defaultCursor = xcb_generate_id(connection);
	xcb_void_cookie_t defaultCookie = xcb_create_glyph_cursor(connection, defaultCursor, cursorFont, cursorFont,
															  DEFAULT_CURSOR_GLYPH, DEFAULT_CURSOR_GLYPH + 1, 0, 0, 0, 0xFFFF, 0xFFFF, 0xFFFF);
	xcb_cursor_t leftRightCursor = xcb_generate_id(connection);
	xcb_void_cookie_t leftRightCookie = xcb_create_glyph_cursor(connection, leftRightCursor, cursorFont, cursorFont,
														        XC_sb_h_double_arrow, XC_sb_h_double_arrow + 1, 0, 0, 0, 0xFFFF, 0xFFFF, 0xFFFF);
	xcb_cursor_t topBottomCursor = xcb_generate_id(connection);
	xcb_void_cookie_t topBottomCookie = xcb_create_glyph_cursor(connection, topBottomCursor, cursorFont, cursorFont,
																XC_sb_v_double_arrow, XC_sb_v_double_arrow + 1, 0, 0, 0, 0xFFFF, 0xFFFF, 0xFFFF);
															 
	/* Cusror glyph setup (corners) */
	xcb_cursor_t topLeftCursor = xcb_generate_id(connection);
	xcb_void_cookie_t topLeftCookie = xcb_create_glyph_cursor(connection, topLeftCursor, cursorFont, cursorFont,
																XC_top_left_corner , XC_top_left_corner + 1, 0, 0, 0, 0xFFFF, 0xFFFF, 0xFFFF);
	xcb_cursor_t bottomLeftCursor = xcb_generate_id(connection);
	xcb_void_cookie_t bottomLeftCookie = xcb_create_glyph_cursor(connection, bottomLeftCursor, cursorFont, cursorFont,
																XC_bottom_left_corner , XC_bottom_left_corner + 1, 0, 0, 0, 0xFFFF, 0xFFFF, 0xFFFF);
	xcb_cursor_t topRightCursor = xcb_generate_id(connection);
	xcb_void_cookie_t topRightCookie = xcb_create_glyph_cursor(connection, topRightCursor, cursorFont, cursorFont,
																XC_top_right_corner , XC_top_right_corner + 1, 0, 0, 0, 0xFFFF, 0xFFFF, 0xFFFF);
	xcb_cursor_t bottomRightCursor = xcb_generate_id(connection);
	xcb_void_cookie_t bottomRightCookie = xcb_create_glyph_cursor(connection, bottomRightCursor, cursorFont, cursorFont,
																XC_bottom_right_corner , XC_bottom_right_corner + 1, 0, 0, 0, 0xFFFF, 0xFFFF, 0xFFFF);
	
	xcb_change_window_attributes(connection, root_window, XCB_CW_CURSOR, &defaultCursor);
	
	/* Main event loop */
	while(true)
	{	
		/* Process new events */
		xcb_generic_event_t *event = xcb_wait_for_event(connection);
		if(event != NULL)
		{	
			switch(event->response_type & ~0x80)
			{	
				/* Handle key press events */
				case XCB_KEY_PRESS:
				{					
					/* Get pressed key and add it to the array*/
					xcb_key_press_event_t *key_press_event = (xcb_key_press_event_t*) event;
					xcb_keycode_t keycode = key_press_event->detail;
					xcb_keysym_t keysym = xcb_key_symbols_get_keysym(key_symbols, keycode, 0);
					combo_keysyms[combo_index] = keysym;
					combo_index++;

					/* Iterate through combinations specified in keybind file */
					int i;
					for(i=0; i<keybinds->num_binds; i++)
					{
						int bind_length = keybinds->bind_lengths[i];
						xkb_keysym_t *combo = keybinds->combos[i];
						
						if(combo_index == bind_length && memcmp(combo_keysyms, combo, sizeof(xkb_keysym_t) * bind_length) == 0)
						{
							process_action(keybinds->actions[i], connection);
						}
					}				
					break;
				}
				
				/* Handle key release events*/
				case XCB_KEY_RELEASE:
				{
					/* Get released key  */
					xcb_key_release_event_t *key_release_event = (xcb_key_release_event_t*) event;
					xcb_keycode_t keycode = key_release_event->detail;
					xcb_keysym_t keysym = xcb_key_symbols_get_keysym(key_symbols, keycode, 0);
					int keysym_index = -1;
					
					/* Find index of released key in combo array */
					for(int i = 0; i < combo_index; i++)
					{
						if(combo_keysyms[i] == keysym)
						{
							keysym_index = i;
							break;
						}
					}

					/* Remove the released key from the combo array */
					if(keysym_index != -1)
					{
						memmove(combo_keysyms + keysym_index, combo_keysyms + keysym_index + 1, (combo_index - keysym_index - 1) * sizeof(xkb_keysym_t));
						combo_index--;
					}
					break;
				}
				
				/* Handle request to map newly created window */
				case XCB_MAP_REQUEST:
				{		
					xcb_map_request_event_t *map_request = (xcb_map_request_event_t *) event;
					xcb_window_t win = map_request->window;
					wormhole_create_window(win, screen, connection);
					xcb_flush(connection);
					break;
				}
				
				/* Handles button press events */
				case XCB_BUTTON_PRESS:
				{
					printf("BUTTON PRESS DETECTED.\n");
					xcb_button_press_event_t *button_event = (xcb_button_press_event_t *) event;
					xcb_window_t win = button_event->event;
					
					start_mouse_x = button_event->event_x;
					start_mouse_y = button_event->event_y;
					
					xcb_set_input_focus(connection, XCB_INPUT_FOCUS_PARENT, win, XCB_CURRENT_TIME);
					button_held = true;
					
					if(left_hovering)
					{
						if(top_hovering)
						{
							top_left_dragging = true;
						}
						
						else if(bottom_hovering)
						{
							bottom_left_dragging = true;
						}
						
						else
						{
							left_dragging = true;
						}
					}
					
					else if(right_hovering)
					{
						if(top_hovering)
						{
							top_right_dragging = true;
						}
						
						else if(bottom_hovering)
						{
							bottom_right_dragging = true;
						}
						
						else
						{
							right_dragging = true;
						}
					}
					
					else if(top_hovering)
					{
						top_dragging = true;
					}
					
					else if(bottom_hovering)
					{
						bottom_dragging = true;
					}
					
					xcb_flush(connection);
					break;
				}
					
				/* Handles button release events*/
				case XCB_BUTTON_RELEASE:
				{	
					xcb_button_press_event_t *button_event = (xcb_button_press_event_t *) event;
					xcb_window_t win = button_event->event;
					xcb_window_t child = wormhole_get_child(win, root_window);
					xcb_set_input_focus(connection, XCB_INPUT_FOCUS_PARENT, child, XCB_CURRENT_TIME);
					
					left_dragging = false;
					right_dragging = false;
					top_dragging = false;
					bottom_dragging = false;
					top_left_dragging = false;
					top_right_dragging = false;
					bottom_right_dragging = false;
					bottom_left_dragging = false;
					button_held = false;
					break;
				}
				
				/* Handles motion notify events */
				case XCB_MOTION_NOTIFY:
				{	
					xcb_motion_notify_event_t *motion_event = (xcb_motion_notify_event_t *) event;
					xcb_window_t p_win = motion_event->event;
					
					xcb_get_geometry_reply_t *g_reply = xcb_get_geometry_reply(connection, xcb_get_geometry(connection, p_win), NULL);
					if(!g_reply)
					{
						printf("[Wormhole] Error - Failed to retreive geometry of focused window.\n");
						break;
					}
					
					int win_x_pos = g_reply->x;
					int win_y_pos = g_reply->y;
					int parent_width = g_reply->width;
					int parent_height = g_reply-> height;
					mouse_x = motion_event->event_x;
					mouse_y = motion_event->event_y;
					free(g_reply);	
					
					left_hovering = false;
					right_hovering = false;
					top_hovering = false;
					bottom_hovering = false;
					
					/* Check if mouse is on border(s) */
					if(!button_held)
					{
						if(mouse_x <= 0) left_hovering = true;
						if(mouse_x >= parent_width) right_hovering = true;
						if(mouse_y <= 0) top_hovering = true;
						if(mouse_y >= parent_height) bottom_hovering = true;
					}
					
					if(left_dragging)
					{
						printf("Left dragging detected");
					}
					
					else if(top_left_dragging)
					{
						printf("Top left dragging detected");
					}
					
					else if(top_dragging)
					{
						printf("Top dragging detected");
					}
					
					else if(top_right_dragging)
					{
						printf("Top right dragging detected");
					}
					
					else if(right_dragging)
					{
						printf("Right dragging detected");
					}
					
					else if(bottom_right_dragging)
					{
						printf("Bottom right dragging detected");
					}
					
					else if(bottom_dragging)
					{
						printf("Bottom dragging detected");
					}
					
					else if(bottom_left_dragging)
					{
						printf("Bottom left dragging detected");
					}
					
					else
					{
						/* Set cursor */
						if(left_hovering)
						{
							if(top_hovering)
							{
								xcb_change_window_attributes(connection, p_win, XCB_CW_CURSOR, &topLeftCursor);
							}
							
							else if(bottom_hovering)
							{
								xcb_change_window_attributes(connection, p_win, XCB_CW_CURSOR, &bottomLeftCursor);
							}
							
							else
							{
								xcb_change_window_attributes(connection, p_win, XCB_CW_CURSOR, &leftRightCursor);
							}
						}
						
						else if(right_hovering)
						{
							if(top_hovering)
							{
								xcb_change_window_attributes(connection, p_win, XCB_CW_CURSOR, &topRightCursor);
							}
							
							else if(bottom_hovering)
							{
								xcb_change_window_attributes(connection, p_win, XCB_CW_CURSOR, &bottomRightCursor);
							}
							
							else
							{
								xcb_change_window_attributes(connection, p_win, XCB_CW_CURSOR, &leftRightCursor);
							}
						}
						
						else if(top_hovering)
						{
							xcb_change_window_attributes(connection, p_win, XCB_CW_CURSOR, &topBottomCursor);
						}
						
						else if(bottom_hovering)
						{
							xcb_change_window_attributes(connection, p_win, XCB_CW_CURSOR, &topBottomCursor);
						}
						
						/* Handle window bar dragging */
						else
						{	
							xcb_change_window_attributes(connection, p_win, XCB_CW_CURSOR, &defaultCursor);
							if(!button_held) break;
							printf("Bar drag detected\n");
							xcb_configure_window(connection,
												 p_win,
												 XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
												 (const uint32_t[]) {win_x_pos + mouse_x - start_mouse_x, win_y_pos + mouse_y - start_mouse_y});
							xcb_flush(connection);
						}
					}
					
					break;
				}
				
				/* Handles shift in window focus */
				case XCB_FOCUS_IN:
				{
					// NOTE: This may cause issues - need to revisit
					xcb_focus_in_event_t *focus_event = (xcb_focus_in_event_t *) event;
					xcb_window_t win = focus_event->event;
					xcb_configure_window(connection,
										 win,
										 XCB_CONFIG_WINDOW_STACK_MODE, 
										 (const uint32_t[]) {XCB_STACK_MODE_ABOVE});
					xcb_flush(connection);
					
					printf("Setting focus to window %u\n", win);
					
					break;
				}
				
				/* Handles mouse leaving window border */
				case XCB_LEAVE_NOTIFY:
				{
					xcb_leave_notify_event_t *leave_event = (xcb_leave_notify_event_t *)event;
					xcb_change_window_attributes(connection, leave_event->event, XCB_CW_CURSOR, &defaultCursor); // reset cursor
					if (leave_event->event == wormhole_get_parent(leave_event->event, root_window)) // check if parent window (will this work? need memcmp?)
					{
						printf("Mouse left the reparented window\n");
						left_hovering = false;
						right_hovering = false;
						top_hovering = false;
						bottom_hovering = false;
						
						// render default cursor
					}
					break;
				}
			}
			free(event);
		}
	}

	xkb_state_unref(state);
	xkb_keymap_unref(keymap);
	xkb_context_unref(ctx);
	
	free(keybinds);
	xcb_disconnect(connection);
	return 0;
}
