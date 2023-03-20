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
#include <inttypes.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-x11.h>
#include "action_processor.h"
#include "keybinds.h"
#include "manager.h"

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
	keybinds_t *keybinds;
	keybinds = read_keybind_configuration();
	printf("[Wormhole] Successfully loaded configuration.\n");
	
	xkb_keysym_t combo_keysyms[MAX_COMBO_LENGTH];
	int combo_index = 0;

	int16_t mouse_x = 0;
	int16_t mouse_y = 0;
	int16_t start_mouse_x = 0;
	int16_t start_mouse_y = 0;
	bool button_held = false;

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
					xcb_button_press_event_t *button_event = (xcb_button_press_event_t *) event;
					xcb_window_t win = button_event->event;
					
					start_mouse_x = button_event->event_x;
					start_mouse_y = button_event->event_y;
					
					xcb_set_input_focus(connection, XCB_INPUT_FOCUS_POINTER_ROOT, win, XCB_CURRENT_TIME);
					button_held = true;
					xcb_flush(connection);
					break;
				}
					
				/* Handles button release events*/
				case XCB_BUTTON_RELEASE:
				{					
					button_held = false;
					break;
				}
				
				/* Handles motion notify events */
				case XCB_MOTION_NOTIFY:
				{	
					if(!button_held)
					{
						break;
					}
				
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
					mouse_x = motion_event->event_x;
					mouse_y = motion_event->event_y;
					free(g_reply);
					
					xcb_configure_window(connection,
										 p_win,
										 XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
										 (const uint32_t[]) {win_x_pos + mouse_x - start_mouse_x, win_y_pos + mouse_y - start_mouse_y});
					xcb_flush(connection);
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
