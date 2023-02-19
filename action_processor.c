#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <xcb/xcb.h>
#include <inttypes.h>

// TODO: remove hardcoded value
int SHIFT_AMOUNT = 10;

/* Function for shifting window */
void shift_action(xcb_connection_t *connection, char direction)
{
	/* Handle errors */
	xcb_get_input_focus_reply_t *reply = xcb_get_input_focus_reply(connection, xcb_get_input_focus(connection), NULL);
	if(!reply) 
	{
		printf("[Wormhole] Error - Failed to locate focused window.\n");	
		return;
	}
	xcb_window_t focused = reply->focus;

	xcb_query_tree_reply_t *p_reply = xcb_query_tree_reply(connection, xcb_query_tree(connection, focused), NULL);
	if(p_reply)
	{
		printf("[Wormhole] Error - failed to retreive parent of focused window\n");
	}
	xcb_window_t parent = p_reply->parent;

	xcb_get_geometry_reply_t *g_reply = xcb_get_geometry_reply(connection, xcb_get_geometry(connection, parent), NULL);
	if(!g_reply)
	{
		printf("[Wormhole] Error - Failed to retreive geometry of focused window's parent.\n");
		return;
	}

	/* Get dimensions and position of focused window */
	int16_t win_x = g_reply->x;
	int16_t win_y = g_reply->y;
	int16_t win_w = g_reply->width;
	int16_t win_h = g_reply->height;

	int shift_x_amount = 0;
	int shift_y_amount = 0;
	
	/* Assign directional shift */
	if(direction == 'L')
	{
		shift_x_amount = shift_x_amount - SHIFT_AMOUNT;
	}
	
	if(direction == 'R')
	{
		shift_x_amount = shift_x_amount + SHIFT_AMOUNT;
	}
	
	if(direction == 'U')
	{
		shift_y_amount = shift_y_amount - SHIFT_AMOUNT;
	}
	
	if(direction == 'D')
	{
		shift_y_amount = shift_y_amount + SHIFT_AMOUNT;
	}
	
	uint32_t values[1];
	values[0] = XCB_EVENT_MASK_ENTER_WINDOW |
				XCB_EVENT_MASK_FOCUS_CHANGE |
				XCB_EVENT_MASK_KEY_PRESS |
				XCB_EVENT_MASK_KEY_RELEASE;
	
	uint32_t geometry[5];
	geometry[0] = win_x + shift_x_amount;
	geometry[1] = win_y + shift_y_amount;
	geometry[2] = win_w;
	geometry[3] = win_h;
	geometry[4] = 5; 
	
	/* Configure window */
	xcb_configure_window(connection, parent, XCB_CONFIG_WINDOW_X |
											  XCB_CONFIG_WINDOW_Y |
											  XCB_CONFIG_WINDOW_WIDTH |
											  XCB_CONFIG_WINDOW_HEIGHT | 
											  XCB_CONFIG_WINDOW_BORDER_WIDTH, geometry);
	xcb_flush(connection);
	xcb_change_window_attributes_checked(connection, parent, XCB_CW_EVENT_MASK, values);
	
	free(g_reply);
	free(p_reply);
	free(reply);
}

/* Method for processing actions triggered by keybinds */
void process_action(char *action, xcb_connection_t *connection)
{	
	/* Process shell commands */
	char *instruction_signifier = "wcmd ";
	if(memcmp(action, instruction_signifier, strlen(instruction_signifier)) != 0)
	{
		/* Default */
		pid_t pid = fork();
		if(pid == 0)
		{		
			setsid();
			char *const args[] = {"/bin/sh", "-c", action, NULL};
			execv("/bin/sh", args);
			printf("[Wormhole] Failed to execute shell command.\n");
			exit(1);	
		}
		
		/* Handle process-forking error */
		if(pid < 0)
		{ 
			printf("[Wormhole] Failed to fork process.\n");
			return;
		}
	}
	
	/* Process Wormhole-specific actions */
	else
	{	
		char* action_string = "wcmd shift_left";
		if(memcmp(action, action_string, strlen(action_string)) == 0)
		{
			shift_action(connection, 'L');
		}
		
		action_string = "wcmd shift_right";
		if(memcmp(action, action_string, strlen(action_string)) == 0)
		{
			shift_action(connection, 'R');
		}
		
		action_string = "wcmd shift_up";
		if(memcmp(action, action_string, strlen(action_string)) == 0)
		{
			shift_action(connection, 'U');
		}
		
		action_string = "wcmd shift_down";
		if(memcmp(action, action_string, strlen(action_string)) == 0)
		{
			shift_action(connection, 'D');
		}
		
		action_string = "wcmd kill";
		if(memcmp(action, action_string, strlen(action_string)) == 0)
		{
			xcb_get_input_focus_reply_t *f_reply = xcb_get_input_focus_reply(connection, xcb_get_input_focus(connection), NULL);
			xcb_window_t win = f_reply->focus;
			if (f_reply != NULL)
			{
				xcb_destroy_window(connection, win);
				xcb_flush(connection);
				free(f_reply);
			}
		}
	}
}
