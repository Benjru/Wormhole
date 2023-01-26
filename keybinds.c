#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_keysyms.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-x11.h>
#include "keybinds.h"

char *KEYBINDS_CONFIGURATION_PATH = "keybinds.conf";
int LINE_BUFFER_SIZE = 200;
int KEYSYM_BUFFER_SIZE = 50;

/* Method for measuring size of keybind file */
int get_keybind_file_size()
{
	FILE *keybind_config_file = fopen(KEYBINDS_CONFIGURATION_PATH,"r");
	if(keybind_config_file == NULL) return 0;
	
	char curr_line[LINE_BUFFER_SIZE];
	char *lineRet;
	int num_lines = 0;
	
	while((lineRet = fgets(curr_line, LINE_BUFFER_SIZE, keybind_config_file)) != NULL)
	{
		if(strlen(lineRet) != 0 && *lineRet != '#' && *lineRet != '\t' && *lineRet != '\n') num_lines++;
	}
	
	fclose(keybind_config_file);
	return num_lines;
}

/* Method for reading in keybind configuration file */
keybinds_t *read_keybind_configuration()
{
	FILE *keybind_config_file = fopen(KEYBINDS_CONFIGURATION_PATH,"r");
	keybinds_t *keybinds = (keybinds_t *) malloc(sizeof(keybinds_t));
	
	/* Handle config not found */
	if(keybind_config_file == NULL)
	{
		printf("[Wormhole] Failed to open keybind file\n");
		return keybinds;
	}
	
	char curr_line[LINE_BUFFER_SIZE];
	char *splitter = ":";
	char *keypress_string;
	char *instruction;
	char *lineRet;
	char **combo_actions;	
	char **keypress_str_arr;
	int *keybind_lengths;
	xkb_keysym_t **key_combos;
	
	/* Allocate memory for main arrays */
	keypress_str_arr = (char **) malloc(sizeof(char *));
	key_combos = (xkb_keysym_t **) malloc(sizeof(xkb_keysym_t *));
	combo_actions = (char **) malloc(sizeof(char *));
	keybind_lengths = (int *) malloc(sizeof(int));
	
	/* Scan config for size data */
	int config_size = get_keybind_file_size();
	int num_keybinds = 0;
	int bind_index = 0;
	
	/* Loop through the contents of the file */
	while((lineRet = fgets(curr_line, LINE_BUFFER_SIZE, keybind_config_file)) != NULL)
	{	
		if(strlen(lineRet) != 0 && *lineRet != '#' && *lineRet != '\t' && *lineRet != '\n')
		{	
			int ksa_size = 0;
			key_combos[bind_index] = (xkb_keysym_t*) malloc(sizeof(xkb_keysym_t));
			combo_actions[bind_index] = (char*) malloc(sizeof(char) * LINE_BUFFER_SIZE);	
			keypress_string = strtok(curr_line, splitter);
			instruction = strtok(NULL, splitter);
			keypress_str_arr = NULL;
			char *curr_key = strtok(keypress_string, "+");
			
			/* Handle case where delimiter is not present */
			if(curr_key == NULL)
			{
				curr_key = keypress_string;
			}
			
			/* Convert string of keys into array of key strings */
			while(curr_key)
			{
				keypress_str_arr = realloc(keypress_str_arr, sizeof(char *) * ++ksa_size);
				key_combos[bind_index] = reallocarray(key_combos[bind_index], ksa_size, sizeof(xkb_keysym_t) * ksa_size);
				keypress_str_arr[ksa_size - 1] = strdup(curr_key);
				curr_key = strtok(NULL, "+");
			}
			keypress_str_arr = realloc(keypress_str_arr, sizeof(char *) * (ksa_size));
			xkb_keysym_t keysym_arr[ksa_size];
			
			/* Convert the array of key strings into an array of keysyms */
			int i;
			for(i=0; i<ksa_size; i++)
			{
				char *str_rep = keypress_str_arr[i];	
				xkb_keysym_t curr_sym = xkb_keysym_from_name(str_rep, XKB_KEYSYM_CASE_INSENSITIVE);
				keysym_arr[i] = curr_sym;
			}

			/* Reallocate arrays and append line data */
			key_combos = reallocarray(key_combos, bind_index + 1, sizeof(xkb_keysym_t *) * (bind_index + 1));		
			combo_actions = reallocarray(combo_actions, bind_index + 1, sizeof(char *) * (bind_index + 1));
			key_combos[bind_index] = (xkb_keysym_t*) malloc(sizeof(xkb_keysym_t) * ksa_size);
			memcpy(key_combos[bind_index], keysym_arr, sizeof(xkb_keysym_t) * ksa_size);
			
			/* Create null-terminated instruction string */
			char nt_instruction[strlen(instruction) + 1];
			strcpy(nt_instruction, instruction);
			nt_instruction[strlen(instruction)] = '\0';
			combo_actions[bind_index] = (char*) malloc(sizeof(char) * strlen(nt_instruction));
			memcpy(combo_actions[bind_index], nt_instruction, sizeof(char) * strlen(nt_instruction));
			
			/* Update bind lengths */
			keybind_lengths[bind_index] = ksa_size;
			keybind_lengths = realloc(keybind_lengths, sizeof(int) * (++bind_index));		
		}
	}
	
	/* Allocate struct arrays */
	keybinds->combos = (xkb_keysym_t **) malloc(sizeof(xkb_keysym_t *) * bind_index);
	keybinds->actions = (char **) malloc(sizeof(char *) * bind_index);
	
	/* Assign struct arrays */
	memcpy(keybinds->combos, key_combos, sizeof(xkb_keysym_t *) * bind_index);
	memcpy(keybinds->actions, combo_actions, sizeof(char *) * bind_index);

	/* Allocate & assign struct size data */
	num_keybinds = bind_index;
	keybinds->num_binds = num_keybinds;
	keybinds->bind_lengths = (int *) malloc(sizeof(int) * num_keybinds);
	memcpy(keybinds->bind_lengths, keybind_lengths, sizeof(int) * num_keybinds);
	
	fclose(keybind_config_file);
	return keybinds;
}