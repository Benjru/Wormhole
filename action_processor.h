#ifndef ACTION_PROCESSOR_H
#define ACTION_PROCESSOR_H

extern int SHIFT_AMOUNT;

void process_action(char *action, xcb_connection_t *connection);
void shift_action(xcb_connection_t *connection);

#endif