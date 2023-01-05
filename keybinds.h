#ifndef KEYBINDS_H
#define KEYBINDS_H

extern char *KEYBINDS_CONFIGURATION_PATH;
extern int LINE_BUFFER_SIZE ;
extern int KEYSYM_BUFFER_SIZE;

/* Struct used for tracking keybinds */
typedef struct 
{	
	xkb_keysym_t **combos;
	char **actions;
	int num_binds;
	int *bind_lengths;
} keybinds_t;

int get_keybind_file_size();
keybinds_t *read_keybind_configuration();

#endif